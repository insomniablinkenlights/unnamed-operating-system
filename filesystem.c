#include "headers/standard.h"
#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/filesystem.h"
#include "headers/terminal.h"
#include "headers/ps2.h"
#include "headers/string.h"
#include "headers/usermode.h"
#include "headers/device.h"
#include "headers/filesystem_internal.h"
#include "headers/perm.h"
#include "headers/qfs.h"
#define kernelFd ((stream*)(current_task_TCB->file_descriptors))
#define kernelFdLen (current_task_TCB->kernelFdLen)
#define root ((inode*)(current_task_TCB->perms->t_root))
uint64_t kernelFdLastClosed = 0x0;
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
		current_task_TCB->perms = cPD(0xFF, GrabInode(0x0)); //this is fundamentally insecure :3
	}
	OpenStdIn();
}
//the streams (for the case of files) map to ids of inodes
//so to read or write takes a fd which grabs the correct function from the table
//and open uses openfilename and pushes it
//TODO: rework kpalloc to have  second version which gives you continuous memory slabs


/* there will be a global list of stdouts/stdins
 * one process will have an stdout bound to that of another process's stdin, or to an output source
 * same for stdin
 * */
/*typedef struct __attribute__((packed)) stdFIFOBUF{
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
}stdIO;*/ //moved to filesystem.h
/*to write to this, grab the pair, check if type == terminal or whatever, check that sizeof buffer is less than bcount, if so write shit, otherwise block until we can*/
/*to read from this, if bcount > 0 read everything, if we still need to read then block until we can, otherwise block*/
/*how do processes decide to bind stdio?*/
stdIO * stdOutBindings = NULL; //linked list similar to our malloc implementation
stdIO * stdInBindings = NULL;
uint8_t stdinSetup = 0;
stdIO * TermSP = NULL;
uint64_t STDOUTStream(stdIO * arguments, uint64_t UNUSED(position), void * buffer, uint64_t len){
	stdIO * pairOut = arguments -> pairOut;
	int toWrite = 0;
	int WP = 0;
	if(pairOut != NULL && pairOut->type == 1){
		//because we're interfacing directly with terminal drivers here, we don't need to do anything specific. Later on this will have to be reworked to interface with abstract drivers.
		write_to_screen(buffer, len);
	}else{
		//we have an actual stream	
		while(len > 0x0){
			acquire_semaphore(arguments->bufferSema); //on interrupt, we get here, get to the end of all this, but never get back to the reader!
			if(arguments->bufferOutEnd->datac != 255){ //we can write into the end
				toWrite = MIN((unsigned char)(255-(char)(arguments->bufferOutEnd->datac)), len);
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
			if(*(arguments->waiter)) PokeT((arguments->waiter)); //we go there, we clear the waiter, we return, we come back (?)
			else nano_sleep(0x1000000);
				      //current_task_TCB->state == STATE_READY??
		}
	}
	return 0;
}
uint64_t STDINStream(stdIO * arguments, uint64_t UNUSED(position), void * buffer, uint64_t len){ //FIFO
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
//	free(((stdIO*)(kernelFd[0].arguments))->bufferOut);
//	free(((stdIO*)(kernelFd[0].arguments)));
	(kernelFd[0].arguments) = TermSP;
	kernelFd[0].function = StdIOStream;
	kernelFd[0].position = 0;
	kernelFd[0].flags = 0;
}
uint64_t OpenStdIn(){ //literally doesn't open stdin
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
		TermSP->type = 1; //TTY
		*(TermSP->waiter) = NULL;
		//bind TermSP to PS2_DRIVER? Will need to do this in some kinda exec()?
	}
	return 0;
}
int checkStream(uint64_t fd){
	if(fd>DIV64_32(0x1000,sizeof(stream))){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	if(kernelFd[fd].function == StdIOStream){
		if(((stdIO*)(kernelFd[fd].arguments))->pairIn->bufferOut->datac>0) return 1;
		return 0;
	}else{
		return 1;
	}
}
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
struct File * openFilename(const char * name);
uint64_t OPEN(char * filename, uint64_t flags){
	struct File * o = openFilename(filename);
	stream * m;
	if(o->type == 0){
		ERROR(ERR_TODO_PROC_ERR,0);
	}else if(o->type == 1){
		struct streamDescriptor * osd = o->content.sd;
		m = osd->opener(osd->arguments);
	}else{
		//if(!checkFPL(o->F_PL, flags)){
		//	ERROR(ERR_TODO_PROC_ERR,0);
		//}
		m = malloc(sizeof(stream));
		m->arguments = malloc(sizeof(uint64_t));
		((void**)m->arguments)[0] = o->content.in;
		m->position = 0x0;
		m->function = FileStream;
		m->flags = flags;

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
stream * openDEV(char * name){
	//more importantly, OPEN has no safeguards against a file ALREADY being open!
	//so should we? nah.
	//maybe for outputs....
	if(strcmp(name, "tty"+CBASE) == 0){ //PS/2 and VGA
		stream * k2 = malloc(sizeof(stream));
		//k2->arguments = malloc(sizeof(uint64_t));
		(k2->arguments)=malloc(sizeof(stdIO));
		((stdIO*)k2->arguments)-> pairIn= TermSP;
		((stdIO*)k2->arguments)-> pairOut= TermSP;
		((stdIO*)k2->arguments)-> bufferOut= malloc(sizeof(stdFIFOBUF));
		((stdIO*)k2->arguments)-> bufferOutEnd= ((stdIO*)(k2->arguments))->bufferOut;
		((stdIO*)k2->arguments)-> bufferOut->datac=0;
		((stdIO*)k2->arguments)-> bufferOut->next=NULL;
		((stdIO*)k2->arguments)-> bufferSema = create_semaphore(1);
		((stdIO*)k2->arguments)-> type= 0; //normal process
		((stdIO*)k2->arguments)-> waiter = malloc(sizeof(thread_control_block*));
		*(((stdIO*)k2->arguments)->waiter) = NULL;
		k2->position = 0x0;
		k2->function = StdIOStream;
		k2->flags = 0x0;
		return k2;
	}else{
		return dOpen(name);
	}
/*	ERROR(ERR_DEV_NF, (uint64_t)name);
	if(strcmp(name, "com"+CBASE) == 0){ //SERIAL
	}*/
}
void inSO(stream * k2, stdIO * a, stdIO * b){
	k2->arguments = malloc(sizeof(stdIO));
	stdIO * m = k2->arguments;
	m->pairIn = a;
	m->pairOut = b;
	m->bufferOut = malloc(sizeof(stdFIFOBUF));
	m->bufferOutEnd = m->bufferOut;
	m->bufferOut->datac = 0;
	m->bufferOut->next = NULL;
	m->bufferSema = create_semaphore(1);
	m->type = 0;
	m->waiter = malloc(sizeof(thread_control_block*));
	*(m->waiter) = NULL;
	k2->position = 0x0;
	k2->flags = 0x0;
	k2->function = StdIOStream;
}
struct streamDescriptor * getStreamDescriptor(const char * name){
	int i = strlen(name);
	while(name[i] != '/' && i>0) i--;
	if(i!=0)i++;
#ifdef rootstreams
	int ex = 0;
	//A rootstream list would be the list of streams allowed to be accessed from the process.
	for(int k = 0; rootstreams[k]; k++){
		if(strcmp(name+i, rootstreams[k]) == 0){
			ex =1;
			 break;
		}
	}
	if(ex == 0) return NULL;
#endif
	struct streamDescriptor * b = malloc(sizeof(struct streamDescriptor));
	b->arguments = malloc(strlen(name+i)+1);
	strcpy(b->arguments,name+i);
	b->opener = openDEV;//TEMPORARY
	return b;
}
struct File * openFilename(const char * name){
	void * target;
	struct File * k = malloc(sizeof(struct File));
	target = getFileFromFilename(root, name);
	if(!target){
		target = getStreamDescriptor(name);
		if(target){
			k->content.sd = target;
			k->type = 1;
		}
		else{
			BREAK((uint64_t)name);
			k->type = 0;
		}
	}
	else{
		k->content.in = target;
		k->type = 2;
	}
	return k;
}
void start_init_task(){ //why the fuck is it in here
	InitKernelFd();
	ckprocA(PS2_DRIVER,0,cPD(0xFF, root));
	while(keyboard_init == 0x0){
		nano_sleep(0x1000000); //TODO: poke
	}
//	while(1){ //just let the driver do its stuff
//		nano_sleep(0x1000000);
//	}
	/*COM_IRQ();
	dProbe("/kmod/serial\0"+CBASE, ""+CBASE); //this is what broke it I believe
	uint64_t serialID = OPEN("/dev/com"+CBASE, 0);
	BREAK(0x242);
	WRITE(serialID, "com"+CBASE, 3);
	BREAK(0x342);
	while(1)nano_sleep(0x10000000);
	CLOSE(serialID);*/
	int m = ExecFile("/sbin/init\0"+CBASE, ""+CBASE, cPD(0xFF, root));
	
	unblock_child(m);
	waitForChildToDie();
	ERROR(ERR_DEADCODE,1);
	//run_EXE("/sbin/init\0"+CBASE);
/*	uint64_t id = OPEN("/TXT\0"+CBASE, 0x0); //for some reason this gives us root instead of txt
	READ(id, UPALLOC(0x0), 0x200); //current segfault: attempting to read into user territory
	asm volatile("xchgw %bx, %bx; xchgw %bx, %bx");	*/
}
uint64_t VERIFY_FLAGS(uint64_t rdx){
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
void BINDR(thread_control_block * b, int fd){ //POSIX hates this one simple trick!
	//literally set b's stdio to be our stdio and delete our stdio
	//TODO: multithreading-incompatible!
	/*stream * dfd = kernelFd[0];
	stream * rfd = (stream*)(b->file_descriptors);
	stdIO * dfds = (stdIO*)(dfd->arguments);
	stdIO * rfds = (stdIO*)(rfd->arguments);
	*rfd = *dfd;*/
	giveSTDIOback(b, current_task_TCB, fd);	
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
void giveSTDIOback(thread_control_block * recipient, thread_control_block * donor, int dfdn){
	stream * rfd = (stream*)(recipient->file_descriptors);
	stream * dfd = (stream*)(donor->file_descriptors);
	//if(!rfd || !dfd){
	//	ERROR(ERR_GSTDB, 0); //honestly we should init fds in ckproc
	//}
	if(dfd[dfdn].function != StdIOStream){
		BREAK(0x48228);
	}
/*	char k = dfdn + '0';
	write_to_screen(&k, 1);
	if(dfd[dfdn].arguments != TermSP){
	}*/
	if(rfd->function != StdIOStream){ //we lost a stdio somewhere in there
		*rfd = dfd[dfdn]; //copy the entire stream object, but now they have the same arguments!
		/*stdIO * rfds = (stdIO*)(rfd->arguments); //rfds = dfds!
		stdIO * dfds = (stdIO*)(dfd->arguments);
		rfds->pairIn->pairOut=rfds; 
		if(rfds->pairOut != (stdIO*)TermSP)
			rfds->pairOut->pairIn = rfds;*/
		memfill(&(dfd[dfdn]), sizeof(stream));
		/*if(((stdIO*)((stream*)recipient->file_descriptors)[0].arguments)->pairOut!=(stdIO*)TermSP)
			((stdIO*)(stream*)recipient->file_descriptors[0].arguments)->pairOut->pairIn=(stdIO*)(((stream*)(donor->file_descriptors))[0].arguments);
		memfill(((stream*)donor->file_descriptors), sizeof(stream));*/
	}else{
		BREAK(0x5792);
	}
}
