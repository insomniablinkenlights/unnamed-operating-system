#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/chs.h"
#include "headers/proc.h"
uint64_t * read(uint64_t LBA, uint64_t disk, uint16_t len){
	//TODO: drive numbers, len > 0x1000
	if(disk != 0x0){
		ERROR(ERR_READ_DISKINV, disk);
	}
	if(len&511 || len>0x1000){
		ERROR(ERR_READ_SIZE, len);
	}
	void * m = KPALLOC();
	void * m1;
	for(uint32_t i = 0; i<DIV64_32(len, 512); i++){
		m1 = floppy_read(LBA+i, len, disk);
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
	if(len&511 || len>0x1000){
		ERROR(ERR_WRITE_SIZE, len);
	}
	for(uint32_t i = 0; i<DIV64_32(len, 512); i++){
		floppy_write(LBA+i, len, (char*)data+512*i, disk);
	}
}
#define LBA_FS_BASE 72
//DO NOT leave it at 0.
//by doing this I have determined that the reason that reading keeps failing IS a problem in the LBA calculations specifically. TODO: test after 18.
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
void format(){
	/*
	char * m = KPALLOC();
	memfill(m, 0x1000);
(	(inode*)m)->chunkaddr1=1;
(	(inode*)m)->chunklen=1;
(	(inode*)m)->perms=0x8;
(	(inode*)m)->owner=0x0;
(	(inode*)m)->group=0x0;
(	(inode*)m)->timestamp=0x0;
	m+=sizeof(inode);
(	(inode*)m)->chunkaddr1=2;
(	(inode*)m)->chunklen=1;
(	(inode*)m)->perms=0x0;
(	(inode*)m)->owner=0x0;
(	(inode*)m)->group=0x0;
(	(inode*)m)->timestamp=0x0;
	memcpy( ((inode*)m)->name, "TXT\0"+CBASE, 4);
	m-=sizeof(inode);
	write(LBA_FS_BASE, 0x0, 512, m);
	memfill(m, 0x1000);
*(	((uint64_t *) m)) = 0x1;
*(	((uint64_t *) m)+1 )= 0x0;
*(	((uint64_t *) m)+2 )= 0xfff; //if the contents are bad, then here we have our problem
	write(LBA_FS_BASE+1, 0x0, 512, m);
	memfill(m, 0x1000);
	memcpy((char*) m, "hello\0"+CBASE, 5); //TODO: double check memcpy works
	write(LBA_FS_BASE+2, 0x0, 512, m);
	P_FREE(m);*/
}
//and now a function that reads the file into a buffer
//we'll implement IO as streams like in unix
typedef struct stream{
	uint64_t (* function)(void *, uint64_t, void *, uint64_t, uint8_t);
	void * arguments;
	uint64_t position;
	uint64_t flags;
}stream;
//which gives you a uint64_t function(void * arguments, uint64_t position, void * buffer, uint64_t len)
//every process has a fd table, where every fd maps to a stream for reading and/or a stream for writing
//for now we'll just have ONE FD for every kernel task
#define kernelFd ((stream*)(current_task_TCB->file_descriptors))
#define kernelFdLen current_task_TCB->kernelFdLen
inode * root = NULL;
uint64_t kernelFdLastClosed = 0x0;
inode * GrabInode(uint64_t id);
void CloseAllFds(){
	//unimplemented
}
void OpenStdIn();
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
		root = GrabInode(0x0);
	}
	OpenStdIn();
}
//the streams (for the case of files) map to ids of inodes
//so to read or write takes a fd which grabs the correct function from the table
//and open uses openfilename and pushes it
//TODO: and for output?
//TODO: rework kpalloc to have  second version which gives you continuous memory slabs
//TODO: KPFREE
inode * GrabInode(uint64_t id){
	int r = sizeof(inode);
	int k2 = id;
	void * m = read(LBA_FS_BASE+ DIV64_32(r*k2,512), 0x0, 512); //TODO: drive numbers
										      //I THINK that read is failing to do the memcpy maybe
	inode * addr = (inode*)(((char*)m)+r*k2); //TERRIBLE hack. WHY does imulq not work?
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
	void * m = read(LBA_FS_BASE+n->chunkaddr1+DIV64_32(position,512), 0x0, 512); //TODO: drive numbers
	memcpy(buffer, m, len);
	P_FREE(m);
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
	}else {
		ERROR(ERR_FILESTREAM_RW,rw);
		return 0x0;
	}
}
#include "headers/terminal.h"
/* there will be a global list of stdouts/stdins
 * one process will have an stdout bound to that of another process's stdin, or to an output source
 * same for stdin
 * */
typedef struct stdIO{
	struct stdIO * pairIn; //pointer to the matching stdIO
	struct stdIO * pairOut;
	void * bufferOut; //if we write multiple bytes in
	uint64_t bcount; //does not go above 0x1000
	uint64_t type;	
}stdIO;
/*to write to this, grab the pair, check if type == terminal or whatever, check that sizeof buffer is less than bcount, if so write shit, otherwise block until we can*/
/*to read from this, if bcount > 0 read everything, if we still need to read then block until we can, otherwise block*/
/*how do processes decide to bind stdio?*/
stdIO * stdOutBindings = NULL; //linked list similar to our malloc implementation
stdIO * stdInBindings = NULL;
uint8_t stdinSetup = 0;
stdIO * TermSP = NULL;
uint64_t STDOUTStream(stdIO * arguments, uint64_t position, void * buffer, uint64_t len){
	//going to assume that any stdout is directly to the terminal for now; later on the arguments will have pipes and shit
	stdIO * pairOut = arguments -> pairOut;
	int toWrite = 0;
	if(pairOut == NULL){ //We're trying to write to a closed stdout
		return 0; //fail silently
	}else if(pairOut == TermSP){
		while(len > 0x0){ //the ONLY time when we use the pair's output as its input
			toWrite = ((0x1000-pairOut->bcount)<len)?(0x1000-pairOut->bcount):(len);
			memcpy((char*)(pairOut->bufferOut)+pairOut->bcount, buffer, toWrite);
			len-=toWrite;
			pairOut->bcount+=toWrite;
			write_to_screen(pairOut->bufferOut,pairOut-> bcount); //TODO: write_to_screen needs to behave more like a unix terminal
			pairOut->bcount=0;
		}
	}else{
		//we have an actual stream	
		while(len > 0x0){
			if(arguments->pairOut == NULL) return 0; //closed in the middle...
			toWrite = ((0x1000-arguments->bcount)<len)?(0x1000-arguments->bcount):(len);
			if(toWrite == 0x0) continue; //TODO: block for IO
			memcpy((char*)(arguments->bufferOut)+arguments->bcount, buffer, toWrite);
			len-=toWrite;
			arguments->bcount+=toWrite;
		}
	}
	return 0;
}
#include "headers/ps2.h"
uint64_t STDINStream(stdIO * arguments, uint64_t position, void * buffer, uint64_t len){
	//position is unused
	stdIO * pairIn = arguments -> pairIn;
	int toRead = 0;
	int bpos = 0;
	if(pairIn == NULL){ //closed stdin
		memfill(buffer, len);
		return 0; //fail silently	
	}else if(pairIn == TermSP){
		while(len > 0x0){ //TODO: ps/2
			if(keyboard_buffer_end == 0x0)  continue;
			((uint8_t*)buffer)[bpos++] = keyboard_buffer[keyboard_buffer_end--].ascii;
			len--;
		}
	}else{
		while(len > 0x0){
			if(arguments->pairIn == NULL) return 0; //closed in the middle...
			toRead = (pairIn->bcount>len)?(len):(pairIn->bcount);
			if(toRead == 0x0) continue; //TODO: block for IO
						    //this will HALT the system if two processes are waiting for stuff at the same time...
			memcpy( (char*)(buffer)+bpos, pairIn->bufferOut, toRead);
			bpos+=toRead;
			memcpy(pairIn->bufferOut, (char*)(pairIn->bufferOut)+toRead, pairIn->bcount-toRead);
			pairIn->bcount -= toRead;
			len -= toRead;
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
		//we need to signal to the other process that we've closed our buffer...
		free(arguments);
		return 0x0;
	}else{
		ERROR(ERR_FILESTREAM_RW, rw);
		return 0x0;
	}
}
void OpenStdIn(){
	if(!stdinSetup){
		stdinSetup = 0x1;
		TermSP = malloc(sizeof(stdIO));
		TermSP->bufferOut = KPALLOC();
		TermSP->bcount = 0;
		//TODO
	}
	stream * m = malloc(sizeof(stream));
	m->function = StdIOStream;
	m->arguments = malloc(sizeof(stdIO));
	((stdIO*)(m->arguments)) -> pairIn= TermSP;
	((stdIO*)(m->arguments)) -> pairOut= TermSP; //should be NULL unless another process tries to bind to us... idk yet
	((stdIO*)(m->arguments)) -> bufferOut= KPALLOC();
	((stdIO*)(m->arguments)) -> bcount= 0;
	((stdIO*)(m->arguments)) -> type= 0; //normal process
	m->position = 0x0;
	m->flags = 0x0;
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
void SEEK(uint64_t fd, uint64_t pos){
	if(fd>kernelFdLen){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].position = pos;
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
#include "headers/string.h"
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
#include "headers/usermode.h"
void start_init_task(){
	create_kernel_task(PS2_DRIVER);
	while(keyboard_init == 0x0){
		nano_sleep(0x1000000);
	}
	InitKernelFd();
	run_EXE("/sbin/init\0"+CBASE);
/*	uint64_t id = OPEN("/TXT\0"+CBASE, 0x0); //for some reason this gives us root instead of txt
	READ(id, UPALLOC(0x0), 0x200); //current segfault: attempting to read into user territory
	asm volatile("xchgw %bx, %bx; xchgw %bx, %bx");	*/
}
uint8_t VERIFY_FLAGS(uint64_t rdx){
	return rdx&0xff; //does NOT verify shit
}
