#include "headers/stdint.h"
#include "headers/standard.h"
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
	uint64_t * memoryaddress;
	if(size==0) ERROR(ERR_MALLOC_SIZE0, 0x0);
	if(MOD64_32(size,64)){
		size+=64-MOD64_32(size,64);
	}
	if(size>4096-64){
		ERROR(ERR_MALLOC_SIZEMAX, 0x0); //TODO: malloc would need to rearrange the pages
	}
	size = DIV64_32(size, 64); //size is in 8quads now :3
	size += 1; //add 8 quads
	uint64_t * mcp = ml1; //mcp format: bitmask, page pointer
			      //if page pointer == null, we ignore bitmask
	uint8_t m = 0;
	uint64_t * NA = NULL; //NA is an entirely free area we found
	uint64_t sizebitmask = (1<<size)-1;
	do {
		for(int i = 0; i<255; i++){
			if(mcp[i*2+1]==0x0){
				if(NA==NULL)
					NA=mcp+2*i; //oh wow we found an area that's unallocated okay awesome
						    //so NA is a pointer to the index of the first unallocated area we find in the whole fuck ass map
				continue;
			}
			if(mcp[i*2]>=sizebitmask){ //(1<<size)-1 = (size) 1 bits, so we check if the bitmask can fit our malloc
				m=0; //m = number of bits we've matched
				for(int j = 0; j<64; j++){ 
					if(mcp[i*2]&(1<<j)){ //if our bitmask is 1 at location j, we've matched a bit
						m++;
						if(m==size){ //we've matched all the bits !!!
							if(~(mcp[i*2]) & sizebitmask<<(j-size+1)){
								ERROR(ERR_FREE_SIZEMISMATCH, mcp[i*2]);
							}
							mcp[i*2] ^= sizebitmask<<(j-size+1); //bitmask moved left by j-size+1, so if e.g. j is 4 and size is 2, we matched 0b11000 so ^= 3<<3
							uint64_t * memoryaddress = (uint64_t*)((char*)(mcp[i*2+1]) + (j-size+1)*8*sizeof(uint64_t) + 8*sizeof(uint64_t));
							if((uint64_t)memoryaddress+size*8-64>(mcp[i*2+1]+0x1000)){
								ERROR(ERR_MALLOC_OVERALLOC, (uint64_t)memoryaddress);
							}
							memoryaddress[-1] = (uint64_t)(mcp+i*2+1); //page pointer plus j-size+1 * 64 quads, e.g. j 4, size 2, give ppt[3]
															    //and set it to the page pointer
							memoryaddress[-2] = j;                     //-2 = j
							memoryaddress[-3] = size;                  //-3 = size
							return (void*) memoryaddress;
						}
					}else{
						m=0; //if we don't match a bit, reset m so we don't accidentally match stuff
					}
				}
			}
		}
		if(mcp[511]!=0x0)mcp=(uint64_t*)(mcp[511]); //now that we're at the end, check if we have more table
	}while(mcp[511]!=0x0);	
	
	//TODO: this SHOULD fix the error where mcp was just leaking into the next memory segment!
	if(NA == NULL){ //we have no unallocated spaces -- allocate one, set NA to the first one, allocate a page to NA
		mcp[511] = (uint64_t)KPALLOC();
		memfill((void*)(mcp[511]), 0x1000);
		NA = (uint64_t*) (mcp[511]);
	} //wow thanks artavazd for pointing out my dead code
	*(NA) = 0xFFFFFFFFFFFFFFFF ^ sizebitmask;
	*(NA+1) = (uint64_t)KPALLOC(); //TODO: MEMORY LEAK!!!
	memfill((void*)(*(NA+1)), 0x1000);
	memoryaddress = (uint64_t*) (*(NA+1) )+8; //i hope...
	memoryaddress[-1] = (uint64_t)(NA+1);
	memoryaddress[-2] = size -1;
	memoryaddress[-3] = size;
	return (void*)memoryaddress;

	/*}else{ //we have an unallocated space -- allocate it and return
		*(NA) = 0xFFFFFFFFFFFFFFFF ^ sizebitmask; //initialise our bitmap so that the first part is our memory to return
		*(NA+1) = (uint64_t)KPALLOC(); //TODO: MEMORY LEAK!!!
		memfill((void*)(*(NA+1)), 0x1000);
					       //why did I put that there? I think maybe because we never deallocate pages...
		memoryaddress = (uint64_t*) (*(NA+1) +8);
		memoryaddress[-1] = (uint64_t)(NA+1); 
		memoryaddress[-2] = size-1; //j=3, size=4: we matched 0b1111
		memoryaddress[-3] = size;
		return (void*)memoryaddress;

	}*/
}
void free(void * ptr){
	//get the pointer to the bitmask:
	if(ptr == NULL){
		ERROR(ERR_FREE_INVALID_PTR, (uint64_t)ptr);
	}
	uint64_t * bitmaskptr = (uint64_t*)(((uint64_t*)ptr)[-1])-1;
	if(bitmaskptr == NULL){
		ERROR(ERR_FREE_INVALID_PTR2, (uint64_t)ptr);
	}
	//get j and size so that we can reconstruct the section of bitmaskptr that we masked
	uint64_t j = ((uint64_t*)ptr)[-2];
	uint64_t size = ((uint64_t*)ptr)[-3];
	if(size == 0x0){
		//double free
		ERROR(ERR_DOUBLE_FREE, (uint64_t)ptr);
	}
	uint64_t resizebitmask = ((1<<size)-1);
	uint64_t resizebitmaskjshift = resizebitmask <<(j-size+1); //imagine size=3, j=5, 0b111000
	if((*bitmaskptr & resizebitmaskjshift)){ //malloc is returning a region within the free?
		ERROR(ERR_FREE_BADMETADATA, *bitmaskptr & resizebitmaskjshift);
	}
	*bitmaskptr ^= resizebitmaskjshift;
	((uint64_t*)ptr)[-3] = 0x0; //mark for doublefree
	((uint64_t*)ptr)[-2] = 0x0;
	((uint64_t*)ptr)[-1] = 0x0;
}
