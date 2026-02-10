#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/filesystem.h"
extern void * tss64; //defined in protk.S
void LTR(uint64_t n);
void ASMS_UM(void * n);
uint64_t get_rspplus1(uint64_t k);
void loadRSP0(uint64_t rsp){
	current_task_TCB->rsp0 = (uint64_t*)rsp; //TODO: this will need to be reloaded whenever we sproc
	char * tomodify = (char*)(&tss64)+0x04+CBASE;
	*(((uint64_t*)tomodify)) = rsp; //not a typo, we don't want to load rsp0 into rsp0. Fuck x86.
	//LTR(0x28);
	//TODO: refresh TSS
}
void switchToUserModeProc(void* UP){
	//ASSUMPTIONS
	//pagetables already loaded at UP
	//user process at UP
	//if two of these get called at the same time, this will break. the obvious solution is to disable the scheduler, but then how do we re-enable it?
	current_task_TCB->PL = 0x3; //write down that we're in usermode
	if((uint64_t)UP>CBASE){
		ERROR(ERR_NOT_UM, (uint64_t)UP);
	}
	//after this we need to initialise fds, initialise memory, initialise rsp0, intitialise registers, sysexit	
	
	//let's actually initialise the memory in P_FREE instead
	//new RSP0 should be unrelated to our rsp0; we never leave the user process
	//so every interrupt makes a new rsp0 if it doesn't cli -- mainly 0x80 as the culprit
	//and it frees that rsp0 after it's done
	loadRSP0(((uint64_t)KPALLOC())+0xff8); //we'll need to clean this up, TODO: memory leak
				     //we need to reload rsp0 every time i think
	ASMS_UM(UP);
	//TODO: page faults :3
	//by which I mean that we need to dynamically allocate user memory as it's needed

}
void run_EXE(char * name){
	//TODO: tasks need to switch between virtual address spaces
	void * initialM = UPALLOC(0x2);
	uint64_t id = OPEN(name, 0x0); 
	READ(id, initialM, 0x200); //TODO: actually read the ENTIRE thing
	CLOSE(id);
	//we SHOULD have no more FDs -- TODO: make proc create KernelFd
	switchToUserModeProc(initialM);
}
void UM_CLEANUP(){
	if(current_task_TCB->PL != 0x3){
		ERROR(ERR_PL_CLEANUP, current_task_TCB->PL);
	}
	//clean up our UM pages, TODO: refactor when we do shared memory
	U_PFREEALL();
	PROC_EXIT();
}
/*void ExecN(){
	InitKernelFd();
	char * M = malloc(0x201);
	M[0x200] = 0;
	while(M[0x200] != 1){ //wait for synchronisation
		READ(0, M, 0x201);
		nano_sleep(0x1000000);
	}
	//create a UM buffer and read chunks to it
	char * KP0 = UPALLOC(0x2);
	uint64_t i = 0;
	while(M[0x200] == 2){
		READ(0, M, 0x201);
		memcpy(KP0+i, M, 0x200);
		i+=0x200;
		if(!(i&0xFFF) && i>=0x1000){
			UPALLOC(0x2); //this is inherently buggy
		}
	}
	READ(0, M, 0x201);
	//do nothing with it; I'm lazy
	switchToUserModeProc(M);
}
uint64_t ExecFile(char * A, void * args){
	//create a task, make a new stdio, bind our new stdio to its stdio, write the file in 200 byte chunks with headers, write a 'DONE' header containing the arguments in place of data, return a process identifier
	//each process needs a pid and we need a hashtable pid -> proc
	//we also need a table for each proc of which pids it's bound to :3
	//i'm thinking that the FD map can be used to map processes as files, kinda like /proc but evil
	thread_control_block * newt = NULL;
	uint64_t AF = OPEN(A, 0x0); //TODO: file arguments
	newt = create_kernel_task(ExecN);
	while(newt->file_descriptors == NULL || ((stream*)(newt->file_descriptors))->function == NULL){
		nano_sleep(0x1000000); //TODO: seriously, TODO: semaphores
	}
	uint64_t NST_FD = OpenStdIn();
	BIND_T_STDIO(current_task_TCB, NST_FD, newt, 0);
	char * M = malloc(0x201);
	M[0x200] = 1;
	WRITE(NST_FD, M, 0x201);
	//okay we've established sync
	//now we can write the whole buffer and then arguments
	SEEK(AF, 0, 2);
	uint64_t AFL = TELL(AF);
	SEEK(AF, 0, 0);
	uint64_t i = 0;
	while(i<AFL){
		READ(AF, M, 0x200);
		M[0x200]=2;
		WRITE(NST_FD, M, 0x201);
		i+=0x200;
	}
	memfill(M, 0x201);
	//data byte!
	M[0x200] = 3;
	//somehow write the arguments?
	return NST_FD;
}*/
struct __attribute__((packed)) ExecArgsInternal {
	char * File;
	char ** arguments;
	int argc;
	SEMAPHORE * sema;
	int done;
};
void ExecN(void * arguments){
	//old process still has a pointer to all our arguments and shit so it can't free and exit until we're done with them
	//TODO: mutexes/ semaphores/ at least SOME kinda synchronisation
	struct ExecArgsInternal * A = arguments;
	acquire_semaphore(A->sema);
	uint64_t AF = OPEN(A->File, 0x0); //TODO: open it with r/x
	SEEK(AF, 0, 2);
	uint64_t AFL = TELL(AF);
	SEEK(AF, 0, 0);
	for(int i = 0; i<=AFL; i+=0x1000){
		UPALLOC(0x2); //this is inherently buggy, TODO: a upalloc function that makes sure we actually just linearly expand US
	}
	for(int i = 0; i<AFL; i+=0x200){
		READ(AF, 0x0, 0x200); //TODO: make a function that just reads the whole file!
	}
	//TODO argc and argv and shit or whatever
	CLOSE(AF);
	release_semaphore(A->sema);
	switchToUserModeProc(0x0);
}
uint64_t ExecFile(char * A, int argc, char ** argv){
	thread_control_block * new = NULL;
	struct ExecArgsInternal * newArgs = malloc(sizeof(struct ExecArgsInternal));
	newArgs->File = A;
	newArgs -> arguments = argv;
	newArgs->argc = argc;
	newArgs->sema = create_semaphore(1); 
	newArgs->done = 0;
	new = ckprocA(ExecN, newArgs);
	while(1){ //this loops until execn grabs the sema
		acquire_semaphore(newArgs->sema);
		if(newArgs->done == 1){
			release_semaphore(newArgs->sema);
			break;
		}
		release_semaphore(newArgs->sema);
	}
	return new->pid;
}
