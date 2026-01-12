#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/filesystem.h"
extern void * tss64; //defined in protk.S
void LTR(uint64_t n);
void ASMS_UM(void * n);
void loadRSP0(){
	*((uint64_t*)((char*)tss64+0x04)) = (uint64_t)(current_task_TCB->rsp); //not a typo, we don't want to load rsp0 into rsp0. Fuck x86.
	LTR(40);
}
void switchToUserModeProc(void* UP){
	//ASSUMPTIONS
	//pagetables already loaded at UP
	//user process at UP
	//if two of these get called at the same time, this will break. the obvious solution is to disable the scheduler, but then how do we re-enable it?
	if((uint64_t)UP>CBASE){
		ERROR(ERR_NOT_UM, (uint64_t)UP);
	}
	//after this we need to initialise fds, initialise memory, initialise rsp0, intitialise registers, sysexit	
	CloseAllFds(); //TODO: taskwise FD switching !!!
	//let's actually initialise the memory in P_FREE instead
	loadRSP0();
	ASMS_UM(UP);
	//TODO: page faults :3
	//by which I mean that we need to dynamically allocate user memory as it's needed

}
void run_EXE(char * name){
	//TODO: tasks need to switch between virtual address spaces
	void * initialM = UPALLOC(0x0);
	uint64_t id = OPEN(name, 0x0); 
	READ(id, initialM, 0x200); //TODO: actually read the ENTIRE thing
	CLOSE(id);
	switchToUserModeProc(initialM);
}
