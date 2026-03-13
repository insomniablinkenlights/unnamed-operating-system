#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/filesystem.h"
#include "headers/flat.h"
#include "headers/string.h"
#include "headers/brk.h"
extern void * tss64; //defined in protk.S
void LTR(uint64_t n);
void ASMS_UM(void * n, uint64_t rsi);
uint64_t get_rspplus1(uint64_t k);
void loadRSP0(uint64_t rsp){
	current_task_TCB->rsp0 = (uint64_t*)rsp; //TODO: this will need to be reloaded whenever we sproc
	char * tomodify = (char*)(&tss64)+0x04+CBASE;
	*(((uint64_t*)tomodify)) = rsp; //not a typo, we don't want to load rsp0 into rsp0. Fuck x86.
	//LTR(0x28);
	//TODO: refresh TSS
}
void switchToUserModeProc(void* UP, int rdi){
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
	ASMS_UM(UP, rdi); //actually rsi

}
void UM_CLEANUP(){
	if(current_task_TCB->PL != 0x3){
		ERROR(ERR_PL_CLEANUP, current_task_TCB->PL);
	}
	//clean up our UM pages, TODO: refactor when we do shared memory
//	U_PFREEALL();
	PROC_EXIT();
}
struct __attribute__((packed)) ExecArgsInternal {
	char * File;
	char * arguments;
	SEMAPHORE * sema;
	int done;
};
void ExecN(void * arguments){
	//old process still has a pointer to all our arguments and shit so it can't free and exit until we're done with them
	//TODO: mutexes/ semaphores/ at least SOME kinda synchronisation
//	BREAK(0x1483);
	struct ExecArgsInternal * A = arguments;
	InitKernelFd();
//current_task_TCB->brk= (uint64_t)malloc(sizeof(prog_mem));
//	acquire_semaphore(A->sema);
//	BREAK((uint64_t)(A->File));
	uint64_t AF = OPEN(A->File, 0x0); //TODO: open it with r/x
	SEEK(AF, 0, 2);
	uint64_t AFL = TELL(AF);
	SEEK(AF, 0, 0);
	void * MEM = KPALLOCS(DIV64_32(AFL,0x1000)+((AFL&0xFFF)?1:0));
	READ(AF, MEM, AFL);
	int rdi;
	void * INI = PF(MEM, AFL, A->arguments, &rdi);
//	BREAK(0x1482);
	P_FREES(MEM, DIV64_32(AFL,0x1000)+((AFL&0xFFF)?1:0));
//	UPALLOC(0x2, (void*)0x0, AFL);
//	READ(AF, 0x0, AFL*0x200);
	//TODO argc and argv and shit or whatever
	CLOSE(AF);
//	release_semaphore(A->sema);
	A->done = 1;
//	BREAK(0x1481);
	block_task(4);
	switchToUserModeProc(INI, rdi);
}
uint64_t ExecFile(char * A, char * argv){
	thread_control_block * new = NULL;
	struct ExecArgsInternal * newArgs = malloc(sizeof(struct ExecArgsInternal));
	newArgs->File = malloc(strlen(A)+1);
	memcpy(newArgs->File, A, strlen(A)+1);
	newArgs -> arguments = malloc(strlen(argv)+1);
	memcpy(newArgs->arguments, argv, strlen(argv)+1);
	newArgs->sema = create_semaphore(1); 
	newArgs->done = 0;
	new = ckprocA(ExecN, newArgs);
	while(1){ //this loops until execn grabs the sema
	//	acquire_semaphore(newArgs->sema);
		if(newArgs->done == 1){
	//		release_semaphore(newArgs->sema);
			break;
		}
	//	release_semaphore(newArgs->sema);
		nano_sleep(0x10000000);
	}
	return new->pid;
}
