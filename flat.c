#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/flat.h"
//flat binary format
void alseg(uint64_t VADD, uint64_t END, uint64_t START, char* fi){
	uint64_t text_off = VADD&0xFFF;
	uint64_t text_size = END-START;
	uint64_t text_size_pages = 0;
	if(text_off == 0){
		if(text_size&0xFFF){
			text_size_pages = (text_size>>12)+1;
		}else{
			text_size_pages = text_size>>12;
		}
	}else{ //offset and size offset can end up adding up to anywhere from 1 to 1FFE! we need to see if we need to allocate one or two pages
		text_size_pages = (text_size>>12)+((text_size&0xFFF)+((text_off >0x1000)?2:1));
	}
	UPALLOC(2, (void*)(VADD^text_off), text_size_pages);
	memcpy((void*)(VADD), ((char*)fi)+START, text_size);
}
void * PF(void * fi, uint64_t filesize){
	//fi is in kernel memory
	PFH * M = (PFH*)fi;
	//check everything
	BREAK(0x2472);
	uint8_t TEXT_EXISTS = 0;
	uint8_t DATA_EXISTS = 0;
	uint8_t BSS_EXISTS = 0;
	if(M->TEXT_END-M->TEXT_START != 0){
		if(M->TEXT_END>M->TEXT_START){
			if(M->TEXT_VADD+M->TEXT_END-M->TEXT_START<CBASE){
				if(M->TEXT_END<filesize){
					//if(!(M->TEXT_VADD & 0xFFF)){
					TEXT_EXISTS = 1;
					//}
				}
			}
		}
	}
	if(!TEXT_EXISTS){
		ERROR(ERR_FLAT_INV, (uint64_t)M);
	}
	if(M->DATA_END-M->DATA_START != 0){
		if(M->DATA_END>M->DATA_START){
			if(M->DATA_VADD+M->DATA_END-M->DATA_START<CBASE){
				if(M->DATA_END<filesize){
					//if(!(M->DATA_VADD & 0xFFF)){
					DATA_EXISTS = 1;
					//}
				}
			}
		}
	}
	if(M->BSS_SIZE){
		if(M->BSS_VADD+M->BSS_SIZE<CBASE){
			if(!(M->BSS_VADD & 0xFFF)){
				BSS_EXISTS=1;
			}
		}
	}
	//TODO: check for overlap between all three sections
	if(TEXT_EXISTS){
		alseg(M->TEXT_VADD, M->TEXT_END, M->TEXT_START, fi);
	}
	if(DATA_EXISTS){
		alseg(M->DATA_VADD, M->DATA_END, M->DATA_START, fi);
	}
	if(BSS_EXISTS){
		UPALLOC(2, (void*)(M->BSS_VADD), MAX(1,(M->BSS_SIZE)>>12));
	}
	return (void*)(M->TEXT_VADD);
}
