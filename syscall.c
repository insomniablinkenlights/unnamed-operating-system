#include "headers/standard.h"
#include "headers/stdint.h"
#include "headers/device.h"
#include "headers/usermode.h"
#include "headers/addresses.h"
#include "headers/filesystem.h"
#include "headers/proc.h"
#include "headers/brk.h"
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
		case 0x5:  //sbrk(increment = rsi)
			prog_mem * k = ((prog_mem*)(current_task_TCB->brk));
			if(k==NULL){
				ERROR(ERR_BINFMT_BROKEN, (uint64_t)k);
			}
			k->end += rsi; //we'll always trigger a pagefault but now we can handle those !!
			rV = k->end;
			break;
		case 0x6: //tell
			rV = TELL(rsi);
			break;
		case 0x7: //exec
			rV = ExecFile(VERIFY_USER((void*)rsi), VERIFY_USER((void*)rdx), VERIFY_PERMS_EXEC((void*)rcx)); //TODO: verify file
			//we want to verify that we can execute the file, and get the perms that we'll be giving it.
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
		case 0xa:
			//wait
			waitForChildToDie();
			rV = 0;
			break;
		case 0xb:
			BINDR(find_child_by_pid(rsi), rdx);
			rV = 0;
			break;
		case 0xc: //signal child
			//rsi == pid of c
			unblock_child(rsi);
			rV = 0;
			break;
		case 0xd:
			if(current_task_TCB->PL == 2){
				d0xD((void*)rsi); //this changes some of the assumptions we make...
				rV = 0;
			}else{
				rV = -1;
			}
			break;
		case 0xe:
			if(current_task_TCB->PL == 2){
				d0xE(rsi, (void *)rdx);
				rV = 0;
			}else{
				rV = -1;
			}
			break;
		case 0xf:
			if(current_task_TCB->PL == 2){
				wait_for_irq(rsi, 0, NULL);
				PIC_sendEOI(rsi);
				rV = 0;
			}else{
				rV = -1;
			}
			break;
		case 0x10: //check stream
			rV = checkStream(rsi);
			break;
		case 0x11: //unblock and wait
			uwait(rsi);
			rV = 0;
			break;
		case 0x12:
			nano_sleep(rsi);
			rV = 0;
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
