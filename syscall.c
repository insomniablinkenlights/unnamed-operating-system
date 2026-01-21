#include "headers/stdint.h"
#include "headers/usermode.h"
#include "headers/addresses.h"
#include "headers/filesystem.h"
uint64_t INT0x80C(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx){
	switch(rdi){
		case 0x0: //read
			if(!VERIFY_USER((void*)(rdx+rcx))) return 0x1;
			READ(rsi, VERIFY_USER((void*)rdx), rcx);
			return 0x0;
		case 0x1: //write
			if(!VERIFY_USER((void*)(rdx+rcx))) return 0x1;
			WRITE(rsi, VERIFY_USER((void*)rdx), rcx);
			return 0x0;
		case 0x2: //open
			return OPEN(VERIFY_USER((void*)rsi), VERIFY_FLAGS(rdx));
		case 0x3: //close
			CLOSE(rsi);
			return 0x0;
		case 0x4: //seek
			SEEK(rsi, rdx);
			return 0x0;
		default:
			return -1;
	}
}
