#include "headers/stdint.h"
#include "headers/addresses.h"
uint64_t * ml1; 
/*
 * malloc list
 * uint64_t usedchunks
 * void * page
 * usedchunks is a bitmap of 64-byte chunks
 * */
void initMalloc(){
	ml1=KPALLOC();
	ml1[511]=0x0;
	ml1[510]=0x0;
}
void * malloc(uint64_t size){
	if(size==0) FAULT();
	if(size%64){
		size+=64-(size%64);
	}
	if(size>4096){
		FAULT(); //TODO: malloc would need to rearrange the pages
	}
	size /= 64;
	uint64_t * mcp = ml1;
	uint8_t m = 0;
	uint64_t * NA = 0x0;
	do {
		for(int i = 0; i<255; i++){
			if(mcp[i*2+1]==0x0){
				if(NA==0x0)
					NA=mcp+16*i;
				continue;
			}
			if(mcp[i*2]>=(1<<size)-1){
				m=0;
				for(int j = 0; j<64; j++){
					if(mcp[i*2]&(1<<j)){
						m++;
						if(m==size){
							mcp[i*2] ^= ((1<<size)-1)<<(j-size);
							return (void*)(mcp[i*2+1]+(j-size)*64);
						}
					}else{m=0;}
				}
			}
		/*	if(__builtin_popcount(mcp[i*2]) >= size){
				if(((1<<size)-1) &( mcp[i*2]>>__builtin_ctzll(mcp[i*2]))==(1<<size -1)){
					return (void*)(__builtin_ctzll(mcp[i*2])*64 + mcp[i*2+1]);
				}
			}*/
		}
		if(mcp[511]!=0x0)mcp=(uint64_t*)(mcp[511]);
	}while(mcp[511]!=0x0);	
	*(NA) = 0xFFFFFFFFFFFFFFFF ^ ((1<<size)-1);

	*(NA+1) = (uint64_t)KPALLOC();
	return (void*)(*(NA+1));
}
