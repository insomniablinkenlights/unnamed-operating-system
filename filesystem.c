#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/chs.h"
#include "headers/proc.h"
#include "headers/filesystem.h"
#include "headers/terminal.h"
#include "headers/ps2.h"
#include "headers/string.h"
#include "headers/usermode.h"
uint64_t * read(uint64_t LBA, uint64_t disk, uint16_t len){
	//TODO: drive numbers, len > 0x1000, queue?
	if(disk != 0x0){
		ERROR(ERR_READ_DISKINV, disk);
	}
	if(len&511){
		ERROR(ERR_READ_SIZE, len);
	}
	void * m = KPALLOCS((len>>12) + 1);
	void * m1;
	for(uint32_t i = 0; i<DIV64_32(len, 512); i++){
		m1 = floppy_read(LBA+i, 512, disk);
		memcpy((char*)m+512*i, m1, 512);
		P_FREE(m1);
	}
	return m;

}
void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data){
	//TODO: drive numbers, len> 0x1000
	if(disk != 0x0){
		ERROR(ERR_WRITE_DISKINV, disk);
	}
	if(len&511){
		ERROR(ERR_WRITE_SIZE, len);
	}
	for(uint32_t i = 0; i<DIV64_32(len, 512); i++){
		floppy_write(LBA+i, len, (char*)data+512*i, disk);
	}
}
#define LBA_FS_BASE 108
//DO NOT leave it at 0.
typedef struct __attribute__((packed)) inode {
	uint64_t chunkaddr1; //chunksize 4kb
	uint64_t chunklen;
	uint16_t perms;
	uint8_t owner;
	uint8_t group;
	uint64_t timestamp; //nanoseconds after jan 1 1970
	char name[32];
}inode; //filename is at the beginning of file and terminated with \0
  //directories have perm bit 0x8 =1, and are lists of 64-bit inode numbers terminated with 0x0
void format(){ //moved into an external file
}
//and now a function that reads the file into a buffer
//we'll implement IO as streams like in unix
// [MOVED INTO HEADERS/FILESYSTEM.H]
//which gives you a uint64_t function(void * arguments, uint64_t position, void * buffer, uint64_t len)
//every process has a fd table, where every fd maps to a stream for reading and/or a stream for writing
//for now we'll just have ONE FD for every kernel task
#define kernelFd ((stream*)(current_task_TCB->file_descriptors))
#define kernelFdLen (current_task_TCB->kernelFdLen)
inode * root = NULL;
uint64_t kernelFdLastClosed = 0x0;
inode * GrabInode(uint64_t id);
void CloseAllFds(){
	//unimplemented
}
void InitKernelFd(){
	//this obviously does vary per-process
	if(current_task_TCB == NULL){
		ERROR(ERR_CTTCBN, 0x0);
	}
	void * M = KPALLOC();
	memfill(M, 0x1000);
	current_task_TCB->file_descriptors=M;
	kernelFdLastClosed=0x0;
	kernelFdLen=DIV64_32(0x1000,sizeof(stream));
	if(root == NULL){
		root = GrabInode(0x0); //broken here! multitasking?
	}
	OpenStdIn();
}
//the streams (for the case of files) map to ids of inodes
//so to read or write takes a fd which grabs the correct function from the table
//and open uses openfilename and pushes it
//TODO: rework kpalloc to have  second version which gives you continuous memory slabs
inode * GrabInode(uint64_t id){
	int r = sizeof(inode);
	int k2 = id;
	void * m = read(LBA_FS_BASE+ DIV64_32(r*k2,512), 0x0, 512); //TODO: drive numbers
								    //this reads the nth block
										      //I THINK that read is failing to do the memcpy maybe
	inode * addr = (inode*)(((char*)m)+r*k2); //TERRIBLE hack. WHY does imulq not work?
						  //TODO: this breaks past 512!!!
	if((void*)addr == m && id != 0){
		//id != 0, sizeof != 0, id*sizeof = 0. ????
		ERROR(ERR_INODE_NE, k2*r);
	}
	if(addr->chunklen==0x0){
		BREAK(LBA_FS_BASE);
		BREAK(DIV64_32(id*sizeof(inode),512));
		BREAK((uint64_t)m);
		BREAK(MOD64_32(id*sizeof(inode), 512));
		ERROR(ERR_INODE_NE, DIV64_32(id*sizeof(inode),512));
	}
	inode * k = malloc(r);
	memcpy(k, addr, r);
	P_FREE(m);
	return k;
}
void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len){
	if((position+len)>512*n->chunklen){
		ERROR(ERR_FILE_PASTBOUND, position);
	}
/*	if(len>0x200){
		ERROR(ERR_READ_SIZE_TEMP, len);
	}*/
	void * m = read(LBA_FS_BASE+n->chunkaddr1+DIV64_32(position,512), 0x0, len); //TODO: drive numbers
	memcpy(buffer, m, len);
	P_FREES(m, (len>>12) + 1);
}
uint64_t FileReaderStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
	//arguments[0] = inode #
	inode * k =GrabInode(((uint64_t*)arguments)[0]); 
	InodeRead(k, position, buffer, len);
	free(k);
	return position+len;
}
uint64_t FileWriterStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
	inode * k =GrabInode(((uint64_t*)arguments)[0]); 
	InodeRead(k, position, buffer, len);
	free(k);
	return position+len;
}
uint64_t FileStream(void * arguments, uint64_t position, void * buffer, uint64_t len, uint8_t rw){
	if(rw == 0x0){
		return FileReaderStream(arguments, position, buffer, len);
	}else if(rw == 0x1){
		return FileWriterStream(arguments, position, buffer, len);
	}else if(rw == 0x2){
		//cleanup
		free(arguments);	
		return 0x0;
	}else if(rw == 0x3){
		inode * k = GrabInode(((uint64_t*)arguments)[0]);
		uint64_t len = k->chunklen*0x200;
		free(k);
		return len;
	}else {
		ERROR(ERR_FILESTREAM_RW,rw);
		return 0x0;
	}
}
/* there will be a global list of stdouts/stdins
 * one process will have an stdout bound to that of another process's stdin, or to an output source
 * same for stdin
 * */
typedef struct __attribute__((packed)) stdFIFOBUF{
	uint64_t data[32];
	uint8_t datac; //with this we can access 256 bytes, so 8*32 quads
	struct stdFIFOBUF* next;
}stdFIFOBUF;
typedef struct stdIO{
	struct stdIO * pairIn; //pointer to the matching stdIO
	struct stdIO * pairOut;
	stdFIFOBUF * bufferOut; //if we write multiple bytes in
	stdFIFOBUF * bufferOutEnd; //does not go above 0x1000
	uint64_t type;	
	SEMAPHORE * bufferSema;
	thread_control_block ** waiter;
}stdIO;
/*to write to this, grab the pair, check if type == terminal or whatever, check that sizeof buffer is less than bcount, if so write shit, otherwise block until we can*/
/*to read from this, if bcount > 0 read everything, if we still need to read then block until we can, otherwise block*/
/*how do processes decide to bind stdio?*/
stdIO * stdOutBindings = NULL; //linked list similar to our malloc implementation
stdIO * stdInBindings = NULL;
uint8_t stdinSetup = 0;
stdIO * TermSP = NULL;
uint64_t STDOUTStream(stdIO * arguments, uint64_t position, void * buffer, uint64_t len){
	stdIO * pairOut = arguments -> pairOut;
	int toWrite = 0;
	int WP = 0;
	if(pairOut == TermSP){
		//because we're interfacing directly with terminal drivers here, we don't need to do anything specific. Later on this will have to be reworked to interface with abstract drivers.
		write_to_screen(buffer, len);
	}else{
		//we have an actual stream	
		while(len > 0x0){
			acquire_semaphore(arguments->bufferSema); //on interrupt, we get here, get to the end of all this, but never get back to the reader!
			if(arguments->bufferOutEnd->datac != 255){ //we can write into the end
				toWrite = MIN(255-arguments->bufferOutEnd->datac, len);
				memcpy((char*)(arguments->bufferOutEnd->data)+arguments->bufferOutEnd->datac,(char*)( buffer)+WP, toWrite);
				arguments->bufferOutEnd->datac+=toWrite;
				WP+=toWrite;
				len-=toWrite;
			}else{ //we need to make the next free bufferOut
				toWrite = MIN(255, len);
				arguments->bufferOutEnd->next = malloc(sizeof(stdFIFOBUF));
				arguments->bufferOutEnd->next->datac = toWrite;
				memcpy(arguments->bufferOutEnd->next->data, (char*)(buffer)+WP, toWrite);
				WP+=toWrite;
				len-=toWrite;
				arguments->bufferOutEnd = arguments->bufferOutEnd->next;
			}
			release_semaphore(arguments->bufferSema); //this is a super tight loop, chances are we don't switch
			PokeT((arguments->waiter)); //we go there, we clear the waiter, we return, we come back (?)
		//	BREAK(0x842); //we never come back here -- we enter userspace, we go back to the read function, etc... this function is STILL in PokeT's unlock_stuff(), because it schedules right back. By the time we get to the scheduler, I have no idea what the states will be. Ideally we would poke the task after going back to waiting for an irq?
				      //current_task_TCB->state == STATE_READY??
		}
	}
	return 0;
}
uint64_t STDINStream(stdIO * arguments, uint64_t position, void * buffer, uint64_t len){ //FIFO
	//position is unused
	stdIO * pairIn = arguments -> pairIn;
	int toRead = 0;
	int bpos = 0;
	int toRelent = 0;
	if(pairIn == NULL){ //closed stdin
		memfill(buffer, len);
		return 0; //fail silently	
	}else{
		while(len > 0x0){
			if(arguments->pairIn == NULL) return 0; //closed in the middle...
			if(toRelent){
			       	wfPokeT(arguments->pairIn->waiter); 
			}
			toRelent = 0;
			acquire_semaphore(arguments->pairIn->bufferSema);
			//read from the first buffer, if it has no data and isn't the last one then free and move
			toRead = MIN(arguments->pairIn->bufferOut->datac, len);
			if(toRead == 0){
				if(arguments->pairIn->bufferOut != arguments->pairIn->bufferOutEnd){ //len isn't 0 obviously... take the first buffer (empty), get rid of it
					stdFIFOBUF * m = arguments->pairIn->bufferOut;
					arguments->pairIn->bufferOut = m->next;
					free(m);
				}else{ 
					toRelent = 1;
				} 
			}else{
				memcpy((char*)(buffer)+bpos, arguments->pairIn->bufferOut->data, toRead);
				memcpy(arguments->pairIn->bufferOut->data, (char*)(arguments->pairIn->bufferOut->data)+toRead, 0xff-toRead); //I'm not sure if this works :3
				arguments->pairIn->bufferOut->datac -= toRead;
				len-=toRead;
				bpos+=toRead;
			}
			release_semaphore(arguments->pairIn->bufferSema); //the problem is that we want to return control to the writer here, but instead it's very likely that we'll catch fire here. I need a form of IPC that can signal on write! 
		}
	}
	return 0;
}
uint64_t StdIOStream(void * arguments, uint64_t position, void * buffer, uint64_t len, uint8_t rw){
	if(rw == 0x0){
		return STDINStream(arguments, position, buffer, len);
	}else if(rw == 0x1){
		return STDOUTStream(arguments, position, buffer, len);
	}else if(rw == 0x2){
		//TODO: we need to signal to the other process that we've closed our buffer...
		free(arguments);
		return 0x0;
	}else if(rw == 0x3){ //ftell on a stdio should return whether that stdio can read IN
			     //this is a dirty hack and we'd ought to make some kind of file struct
		if(((stdIO*)arguments)->pairIn != NULL){
			return 1;
		}
		return 0;
	}else{
		ERROR(ERR_FILESTREAM_RW, rw);
		return 0x0;
	}
}
void PS2_DRIVER_BIND_STDIO(){
	//assume stdio already exists and is unbound
	free(((stdIO*)(kernelFd[0].arguments))->bufferOut);
	free(((stdIO*)(kernelFd[0].arguments)));
	(kernelFd[0].arguments) = TermSP;
}
uint64_t OpenStdIn(){
	if(!stdinSetup){
		stdinSetup = 0x1;
		TermSP = malloc(sizeof(stdIO));
		memfill(TermSP, sizeof(stdIO));
		TermSP->bufferOut = malloc(sizeof(stdFIFOBUF));
		memfill(TermSP->bufferOut, sizeof(stdFIFOBUF)); // clears stuff
		TermSP->bufferOutEnd = TermSP->bufferOut;
		TermSP->bufferOut->datac = 0; //TODO: this might have a fencepost
		TermSP->bufferOut->next = NULL;
		TermSP->bufferSema = create_semaphore(1); //can only be read or write
		TermSP->waiter = malloc(sizeof(thread_control_block*));
		*(TermSP->waiter) = NULL;
		//bind TermSP to PS2_DRIVER? Will need to do this in some kinda exec()?
	}
	stream * m = malloc(sizeof(stream));
	m->function = StdIOStream;
	m->arguments = malloc(sizeof(stdIO));
	((stdIO*)(m->arguments)) -> pairIn= TermSP;
	((stdIO*)(m->arguments)) -> pairOut= TermSP; //should be NULL unless another process tries to bind to us... idk yet
	((stdIO*)(m->arguments)) -> bufferOut= malloc(sizeof(stdFIFOBUF));
	((stdIO*)(m->arguments)) -> bufferOutEnd= ((stdIO*)(m->arguments))->bufferOut;
	((stdIO*)(m->arguments)) -> bufferOut->datac=0;
	((stdIO*)(m->arguments)) -> bufferOut->next=NULL;
	((stdIO*)(m->arguments)) -> bufferSema = create_semaphore(1);
	((stdIO*)(m->arguments)) -> type= 0; //normal process
	((stdIO*)(m->arguments)) -> waiter = malloc(sizeof(thread_control_block*));
	*((((stdIO*)m->arguments))->waiter) = NULL;
	m->position = 0x0;
	m->flags = 0x0;
	if(kernelFd[0].function != NULL){
		ERROR(ERR_STDIO_NONZ, 0);
	}
	while(kernelFdLastClosed<kernelFdLen && kernelFd[kernelFdLastClosed].function != NULL){
		kernelFdLastClosed++;
	}
	if(kernelFdLastClosed == kernelFdLen){
		kernelFdLastClosed = 0x0;
		while(kernelFdLastClosed<kernelFdLen && kernelFd[kernelFdLastClosed].function != NULL){
			kernelFdLastClosed++;
		}
		if(kernelFdLastClosed == kernelFdLen){
			ERROR(ERR_FD_TOOHIGH, kernelFdLastClosed);
		}
	}
	memcpy(kernelFd+kernelFdLastClosed, m, sizeof(stream));
	free(m);
	return kernelFdLastClosed;

}
stream * OpenFilename(inode * basedir, char * filename, uint64_t flags);
void READ(uint64_t fd, void * buffer, uint64_t len){
	if(fd>DIV64_32(0x1000,sizeof(stream))){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].position = (kernelFd[fd].function)(kernelFd[fd].arguments, kernelFd[fd].position, buffer, len, 0x0);
}
void WRITE(uint64_t fd, void * buffer, uint64_t len){
	if(fd>DIV64_32(0x1000,sizeof(stream))){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].position = (kernelFd[fd].function)(kernelFd[fd].arguments, kernelFd[fd].position, buffer, len, 0x1);
}
void SEEK(uint64_t fd, uint64_t pos, uint64_t WHENCE){
	if(fd>kernelFdLen){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	if(WHENCE == 0x0){
		kernelFd[fd].position = pos;
	}else if(WHENCE == 0x1){
		kernelFd[fd].position = kernelFd[fd].position+pos;
	}else{
		kernelFd[fd].position = (kernelFd[fd].function)(kernelFd[fd].arguments, kernelFd[fd].position, NULL, 0x0, 0x3);
	}
}
uint64_t TELL(uint64_t fd){
	if(fd>kernelFdLen){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	return kernelFd[fd].position;
}
uint64_t OPEN(char * filename, uint64_t flags){
	stream * m = OpenFilename(root, filename, flags);
	while(kernelFdLastClosed<kernelFdLen && kernelFd[kernelFdLastClosed].function != NULL){
		kernelFdLastClosed++;
	}
	if(kernelFdLastClosed == kernelFdLen){
		kernelFdLastClosed = 0x0;
		while(kernelFdLastClosed<kernelFdLen && kernelFd[kernelFdLastClosed].function != NULL){
			kernelFdLastClosed++;
		}
		if(kernelFdLastClosed == kernelFdLen){
			ERROR(ERR_FD_TOOHIGH, kernelFdLastClosed);
		}
	}
	memcpy(kernelFd+kernelFdLastClosed, m, sizeof(stream));
	free(m);
	return kernelFdLastClosed;

}
void CLOSE(uint64_t fd){
	if(fd>kernelFdLen){ //TODO: fencepost
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].function(kernelFd[fd].arguments, kernelFd[fd].position, NULL, 0x0, 0x2); //free it all up
	kernelFd[fd].function = NULL;
	kernelFdLastClosed = fd;
}
stream * OpenFilename(inode * basedir, char * filename, uint64_t flags){
	//   filename:    /DIR/DIR/DIR.../file or /file or /DIR/ or /DIR/DIR/
	//   split filename into directories ... filename
	char ** directories = NULL;
	char * name = NULL;
	int slashcount = 0;
	int i = 0;
	int e = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	char * FN2 = NULL;
	FN2++;
	if(FN2 != NULL+1){ //genuinely why the FUCK does this work
		ERROR(ERR_OPEN_TWO, (uint64_t)FN2);
	}
	while(filename[i] != 0x0){
		if(filename[i] == '/'){
			slashcount++;
			while(filename[i] == '/')i++;
			if(filename[i] == 0x0) ERROR(ERR_FILE_DIRSTREAM, (uint64_t)filename);
		}else{
			i++;
		}
		e = i; //TODO: fencepost error?
		       //filename[e] = 0x0
	}
	slashcount --; //root
	if(slashcount <0){
		slashcount=0;
	}
	i = 1; //TODO: breaks if starts with two slashes
	if(slashcount != 0x0){
		//right here we enter an infinite loop...
		directories = malloc(slashcount*sizeof(char*));
		k = i;
		l = i;
		j=0; //we do this earlier, but I assume that I'll break it and forget to fix this here because I don't read my own comments. If it still manages to break in the future I am very sorry but it is all your fault.
		while(j<slashcount){ //TODO: probably doesn't work at all; supposed to split the string
			k = i;
			l = i;
			while(filename[i] != '/'){
				i++;
				if(filename[i] == '\0'){
					ERROR(ERR_FILE_OPEN_SLASHCOUNT, 0x0);
				}
			}
			directories[j] = malloc(i-k);
			while(l<i){
				if(l-k < 0){
					ERROR(ERR_FILE_OPEN_WHY, 0x0);
				}
				directories[j][l-k] = filename[l];
				l++;
			}
			i++; //THIS SPLITS CORRECTLY
			j++;
		}
	}
	i = e;
	while(filename[i-1] != '/'){
		i--;
	}
	//i is now the first character in the actual string
	name = malloc(e-i+1); //I think malloc is broken... 3:
	memcpy(name, filename+i, e-i+1); //TODO: fencepost?
	name[e-i] = 0x0; //for some reason it's not doing it
	if(strcmp(name, filename+i)){
		ERROR(ERR_SPLIT_FAIL, (uint64_t)name);
	}
//	name ++; //if this fixes it i will die
	// scan through all files in basedir, find one with correct name, repeat

	
	inode * curdir = malloc(sizeof(inode));
	memcpy(curdir, basedir, sizeof(inode));
	j=0;
	k = 0;
	int p = 1;
	void * m = KPALLOC();
//	void * m2 = KPALLOC();
	inode * n = NULL;
	while(j<slashcount){ //loop through all directories in path
		l = 0;
		InodeRead(curdir, 0x0, m, 0x200); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
		p=1;
		while(((uint64_t*)m)[l] != 0x0){
			//loop through inodes indicated by directory
			//get directory indicated by this inode, check name equality
			if(n!=NULL){
		//		free(n); n=NULL;
			}
			n=GrabInode(((uint64_t*)m)[l]); //grab l'th inode in directory
			p=strcmp(n->name, directories[j]);
			if(p==0){
				break; //it's this one
			}
			l++;
		}
		if(p){
			ERROR(ERR_IN_DIR_DNE, (uint64_t)filename);
		}
	//	free(curdir); 
					//IM no longer freeing shit to stop this fuck ass double free
		curdir = n;
		j++;
	}

	if(strcmp(name, filename+i)){
		ERROR(ERR_SPLIT_FAIL, (uint64_t)name);
	}
	//finally, find the entry in our CURDIR which contains the thingy
	k = 0;
	l = 0;
	InodeRead(curdir, 0x0, m, 0x200); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
					   //
//	while(((uint64_t*)m)[k++]!=0x0); //I'm not even going to bother parsing the name of curdir because we already did it.
	p=1;
	while(((uint64_t*)m)[l] != 0x0){ //m is a LIST of inodes.
		//get file indicated by this inode, check name equality
	//	if(n!=NULL)free(n); //okay so our new fucking problem is that N is 0x7 ???????????????? why the FUCK would it be 0x7..... im gonna kms im gonna km fucking s rn
		n=GrabInode(((uint64_t*)m)[l]); //should give 0x1
						  //this FINALLY WORKS !!! but next up, "TXT" still doesn't match bc the slash wasn't separated properly TODO
	//	InodeRead(n, 0x0, m2, 0x1000);
		p=strcmp(n->name, name);
		if(p==0){
			break;
		}
		l++;
	}
	if(p){
		ERROR(ERR_IN_FILE_DNE,(uint64_t) name);
	}
	
	stream * k2 = malloc(sizeof(stream));
	k2->arguments = malloc(sizeof(uint64_t));
	((uint64_t*)k2->arguments)[0]=((uint64_t*)m)[l];
	k2->position = 0x0;
	k2->function = FileStream;
	k2->flags = flags;

	P_FREE(m);
//	P_FREE(m2);
/*	for(;slashcount>1;slashcount--){
		free(directories[slashcount-1]);
	}
	if(slashcount>0)free(directories);
	free(name);
	if(curdir != basedir)free(curdir);
	if(n!=NULL)free(n);*/
	return k2;
}
void start_init_task(){ //why the fuck is it in here
	create_kernel_task(PS2_DRIVER);
	while(keyboard_init == 0x0){
		nano_sleep(0x1000000); //TODO: poke
	}
//	while(1){ //just let the driver do its stuff
//		nano_sleep(0x1000000);
//	}
	InitKernelFd();
	int m = ExecFile("/sbin/init\0"+CBASE, 0, NULL);
	unblock_child(m);
	waitForChildToDie();
	ERROR(ERR_DEADCODE,1);
	//run_EXE("/sbin/init\0"+CBASE);
/*	uint64_t id = OPEN("/TXT\0"+CBASE, 0x0); //for some reason this gives us root instead of txt
	READ(id, UPALLOC(0x0), 0x200); //current segfault: attempting to read into user territory
	asm volatile("xchgw %bx, %bx; xchgw %bx, %bx");	*/
}
uint8_t VERIFY_FLAGS(uint64_t rdx){
	return rdx&0xff; //does NOT verify shit
}
void BIND_T_STDIO(thread_control_block * A, uint64_t FD0, thread_control_block * B, uint64_t FD1){
	//TODO: check against FDLEN
	stream * ST1 = ((stream*)(A->file_descriptors))+FD0;
	stream * ST2 = ((stream*)(B->file_descriptors)) +FD1;
	if(ST1->function != StdIOStream || ST2->function == StdIOStream){
		ERROR(ERR_MISP_BINDT, 0);
	}
	//bind A's stdout to B's stdin
	((stdIO*)((ST2->arguments))) ->pairIn = ST1->arguments;
	((stdIO*)((ST1->arguments))) ->pairOut = ST2->arguments;
}
void BIND_HANDLES(uint64_t FD0, uint64_t FD1){
	//TODO: check against FDLEN
	if(kernelFd[FD0].function != StdIOStream || kernelFd[FD1].function != StdIOStream){
		ERROR(ERR_MISP_BINDH, 0);
	}
	stdIO * IO1 = (stdIO*)(kernelFd[FD0].arguments);
	stdIO * IO2 = (stdIO*)(kernelFd[FD1].arguments);
	IO2->pairIn = IO1;
	IO1->pairOut = IO2;
}
void BINDR(thread_control_block * b){ //POSIX hates this one simple trick!
	//literally set b's stdio to be our stdio and delete our stdio
	//TODO: multithreading-incompatible!
	/*stream * dfd = kernelFd[0];
	stream * rfd = (stream*)(b->file_descriptors);
	stdIO * dfds = (stdIO*)(dfd->arguments);
	stdIO * rfds = (stdIO*)(rfd->arguments);
	*rfd = *dfd;*/
	giveSTDIOback(b, current_task_TCB);	
	//*(stdIO*)(((stream*)(b->file_descriptors))[0].arguments) = ;
/*	dfd = rfd;
	dfds = rfds;
	
	((stdIO*)kernelFd[0].arguments)->pairIn->pairOut=(stdIO*)(((stream*)(b->file_descriptors))[0].arguments);
	if(((stdIO*)kernelFd[0].arguments)->pairOut!=(stdIO*)TermSP)
		((stdIO*)kernelFd[0].arguments)->pairOut->pairIn=(stdIO*)(((stream*)(b->file_descriptors))[0].arguments);
	kernelFd[0].arguments = NULL;
	kernelFd[0].function = NULL;*/
	//this should do it
}
void giveSTDIOback(thread_control_block * recipient, thread_control_block * donor){
	stream * rfd = (stream*)(recipient->file_descriptors);
	stream * dfd = (stream*)(donor->file_descriptors);
	//if(!rfd || !dfd){
	//	ERROR(ERR_GSTDB, 0); //honestly we should init fds in ckproc
	//}
	if(rfd->function != StdIOStream){ //we lost a stdio somewhere in there
		*rfd = *dfd; //copy the entire stream object, but now they have the same arguments!
		/*stdIO * rfds = (stdIO*)(rfd->arguments); //rfds = dfds!
		stdIO * dfds = (stdIO*)(dfd->arguments);
		rfds->pairIn->pairOut=rfds; 
		if(rfds->pairOut != (stdIO*)TermSP)
			rfds->pairOut->pairIn = rfds;*/
		memfill (dfd, sizeof(stream));
		/*if(((stdIO*)((stream*)recipient->file_descriptors)[0].arguments)->pairOut!=(stdIO*)TermSP)
			((stdIO*)(stream*)recipient->file_descriptors[0].arguments)->pairOut->pairIn=(stdIO*)(((stream*)(donor->file_descriptors))[0].arguments);
		memfill(((stream*)donor->file_descriptors), sizeof(stream));*/
	}
}
