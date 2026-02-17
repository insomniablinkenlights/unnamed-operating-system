#include "headers/stdint.h"
#include "headers/usermode.h"
#include "headers/addresses.h"
#include "headers/filesystem.h"
#include "headers/proc.h"
uint64_t INT0x80C(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx){ //interrupts are disabled
	uint64_t rV = 0;
	CLI();
	uint64_t * oldrsp0 = current_task_TCB->rsp0;
	uint64_t * newrsp0 = KPALLOC();
	loadRSP0(((uint64_t)newrsp0)+0xff8);
	STI();
	switch(rdi){
		case 0x0: //read
			//this requires us to renable interrupts: save the rsp0, make a new one, enable interrupts, call our shit, disable interrupts, get rid of our rsp0 -- actually, most of them do!
			READ(rsi, VERIFY_USER((void*)rdx), rcx); //we might switch to another usermode task during this... that has problems that I'm not thinking about
			rV= 0x0;
			break;
		case 0x1: //write
			WRITE(rsi, VERIFY_USER((void*)rdx), rcx);
			rV = 0x0;
			break;
		case 0x2: //open
			rV = OPEN(VERIFY_USER((void*)rsi), VERIFY_FLAGS(rdx));
			break;
		case 0x3: //close
			CLOSE(rsi);
			rV = 0x0;
			break;
		case 0x4: //seek
			SEEK(rsi, rdx, rcx);
			rV = 0x0;
			break;
		case 0x5: //extend memory, should always return the next seg
			rV = (uint64_t)UPALLOC(0x2, VERIFY_USER((void*)rsi), rdx);
			break;
		case 0x6: //tell
			rV = TELL(rsi);
			break;
		case 0x7: //exec
			//for this, arguments need to be added to cktask?
			rV = ExecFile(VERIFY_USER((void*)rsi), VERIFY_USER((void*)rdx)); //TODO: verify file
			break;
		case 0x8: //bind
			BIND_HANDLES(rsi, rdx);
			rV = 0;
			break;
		case 0x9: //exit
			//we'll always be inside the process that called exit, but check just in case
			UM_CLEANUP();
			ERROR(ERR_DEADCODE, 0);
			rV = -1; //should NOT get to this statement!
			break;
		default:
			ERROR(ERR_INT, rdi);
			rV = -1;
			break;
	}
	CLI();
	P_FREE(newrsp0);
	loadRSP0((uint64_t)oldrsp0);
	return rV;

}
