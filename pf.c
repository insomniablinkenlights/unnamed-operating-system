#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/brk.h"
void PAGE_FAULT(uint64_t virt_ad, uint64_t errc){
	//error code is padded with 32 zero bits
//	BREAK(0x9a3);
//	BREAK(virt_ad);
//	BREAK(errc);
	if(errc & 1){ //protection violation
		BREAK(virt_ad);
		ERROR(ERR_PAGE_FAULT, errc);
	}
	//we only want to make the page if and only if it's a section within the brk that is nonexistent due to a read or a write
	prog_mem * brk = (prog_mem*)(current_task_TCB->brk);
	if(brk == NULL){
		BREAK(virt_ad);
		ERROR(ERR_PAGE_FAULT, errc);
	}
	if(virt_ad>=brk->start && virt_ad<=brk->end){
		UPALLOC(2, (char*)(virt_ad&(~0xFFF)), 1);
	}else{
		BREAK(virt_ad);
		ERROR(ERR_PAGE_FAULT, errc);
	}
}
