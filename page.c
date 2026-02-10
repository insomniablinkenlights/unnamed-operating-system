#include "headers/stdint.h"
#include "headers/addresses.h"
#define PTV_MEMORYOVERHEAD 0
void FLUSH_TLB();
void FLUSH_TLB2();

uint64_t * manualptVlookup(uint64_t AD){ //TODO: measure cpu usage
	//USE: 0x1c000
	uint64_t offset = (((MBASE-CBASE)>>12)+0x1c)<<3;
	if(offset>=0x1000){
		ERROR(ERR_MBASE_TOOHIGH, offset);
	}
	*((uint64_t*)(MBASE+0x13000+offset)) = AD|0x3;	
	FLUSH_TLB();
	return (uint64_t*)(MBASE+0x1c000);
}
uint64_t get_pml4V(int pml4){ //return value: integer content of pml4
			
	return ((uint64_t*)(MBASE+0x10000))[pml4];
}
uint64_t get_pdptV(int pml4, int pdpt){ //return value: integer content of pdpt
	return manualptVlookup((get_pml4V(pml4)&(~0xFFF)))[pdpt];
}
uint64_t get_pdeV(int pml4, int pdpt, int pde){ //return value: integer content of pde
	return manualptVlookup((get_pdptV(pml4,pdpt)&(~0xFFF)))[pde];
}uint64_t * get_pdptVP(int pml4, int pdpt){ //return value: pointer to first guy in pdpt
	return manualptVlookup(get_pdptV(pml4, pdpt)&(~0xFFF));
}uint64_t * get_pml4VP(int pml4){ //return value: pointer to first guy in pml4
	return manualptVlookup(get_pml4V(pml4)&(~0xFFF));
}

uint64_t * get_pdeVP(int pml4, int pdpt, int pde){ //return value: pointer to first guy in pde
	return manualptVlookup(get_pdeV(pml4, pdpt, pde)&(~0xFFF));
}
uint64_t get_ptV(int pml4, int pdpt, int pde, int pt){
	//this would need a physical to virtual translation layer, or a separate structure mirroring the tree but with virtual addresses
	//the stupid thing to do would be to have a single pt which we know the address of that points wherever we want
		//set address to physical, reload cr3, get content from there -- VERY expensive per lookup
	//or we could have some kind of table for the pts (double memory overhead) but manually lookup pde/pdpt/pml4 when and only when we need to
#if PTV_MEMORYOVERHEAD==0
	return manualptVlookup((get_pdeV(pml4,pdpt,pde)&(~0xFFF)))[pt];
#else
	ERROR(PAGE_ALLOC_SHOULDNOTEXIST,0x0); // unimplemented -- illusion of choice :3
#endif
}
uint64_t construct_virtual_address(uint64_t pml4, uint64_t pdpt, uint64_t pde, uint64_t pt){
	return (uint64_t)(pt*0x1000+pde*0x1000*512+pdpt*0x1000*512*512+pml4*0x1000*512*512*512);
}
uint8_t PA_CALLING = 0x0;
void * UP_ALLOC3(uint8_t FLAGS);
void * KP_ALLOC3();
void * KPALLOC(){ // THIS FUNCTION BREAKS IF CALLED TWICE
	// 0x16000 contains a stack of 64 bit page virtual addresses
	uint64_t k = 0x0;
	uint16_t AN = 0x0;
	for(int i = 0; i<512; i++){
		AN = i;
		if(((uint64_t*)(MBASE+0x16000))[i] != 0x0){
			k = ((uint64_t*)(MBASE+0x16000))[i];
			((uint64_t*)(MBASE+0x16000))[i] = 0x0;
			break;
		}
	}
	if(k == 0x0){
		ERROR(ERR_KPALLOC_NOFREE,0x0);
	}
	if(AN >= 32){
		if(!PA_CALLING){
			for(int i = 0; i<64; i++){
				if(((uint64_t*)(MBASE+0x16000))[i]!=0x0) continue;
				((uint64_t*)(MBASE+0x16000))[i] = (uint64_t)KP_ALLOC3();
			}
		}
	}
	return (void*)k;
}
void * UPALLOC(uint8_t FLAGS){ 
	return UP_ALLOC3(FLAGS);
}
void * PL_MAP_START = (void*)(MBASE+0x15000);
void * LL_NV(uint64_t index, void * PLM){ //we want to return the index'th quad in a linked list PLM
	if(index > 510){ //it's not in this list
		if(((uint64_t*)PLM)[511] == 0x0){ //we have no next list
			((uint64_t*)PLM)[511] = (uint64_t)KPALLOC(); //make a new one
			memfill((uint64_t*)(((uint64_t*)(PLM))[511]), 0x1000); //initialise it
		}
		return LL_NV(index-511, (void*)(((uint64_t*)PLM)[511])); //say our index is 511, we get the first element in the next list
	}
	// it is in this list
	return (void*)((char*)(PLM)+sizeof(uint64_t)*index); //a pointer to the 0th, plus 8 times the index
}
void PL_SV(uint64_t index, uint8_t val){ //lowkey this might be the problem 3:
	if(val == 0x1) {
		*(uint64_t*)LL_NV(index>>6, PL_MAP_START) |= 1 << (index&63); //index >> 6 == index / 64
									      //1<<63 should be below the limit
	}else{
		*(uint64_t*)LL_NV(index>>6, PL_MAP_START) &= ~(1 << (index&63)); 
	}
}
uint64_t PL_FN(){ //this doesn't work and i have no idea why!
	uint64_t i = 0;
	uint64_t * PT = PL_MAP_START;
	while(1){
		for(int j = 0; j<511; j++){
			if(PT[j] != 0xFFFFFFFFFFFFFFFF){ //there's an unset bit somewhere in here
			//	while(!((PT[j]>>((i++)%64))&0x1)); return i;
				int m = 0;
				while((PT[j]&(1<<m)) != 0x0){
					m++;
				}
				if(m != __builtin_ctzll(~PT[j])){
					ERROR(ERR_PL_FN_MISMATCH, m);
				}
				return i+m;
				//return i + __builtin_ctzll(~PT[j]);
			}
			i+=64; //increase the bit index by 64 so we get the next one
		}
		//we didn't match any in this table, go to the next
		if(PT[511]==0x0){ //if we have no next
			PT[511]=(uint64_t)KPALLOC(); //make one
			memfill((uint64_t*)(PT[511]), 0x1000); //and initialise it
		}
		PT=(uint64_t*)(PT[511]); //and then move forward
	}
}
void P_ALLOC3_SETUP(){
	PA_CALLING=0x0;
	memfill((uint64_t*)(MBASE+0x16000), 0x1000);
	memfill((uint64_t*)(MBASE+0x17000), 0x1000);
	memfill ((uint64_t*)(MBASE+0x15000), 0x1000); //intitialise PL
	for(int i = 0; i<512; i++){
		PL_SV(i, 1);
	}
	((uint64_t*)(MBASE+0x16000))[501]=MBASE+0x19000; // pde list plus pdpt list is two extra pages to start
	((uint64_t*)(MBASE+0x16000))[502]=MBASE+0x1A000;
	((uint64_t*)(MBASE+0x16000))[503]=MBASE+0x1E000;
	((uint64_t*)(MBASE+0x16000))[500]=MBASE+0x1F000;
	//manualptVlookup() confirmed to work.
	//fill up the rest of the list...
	((uint64_t*)(MBASE+0x16000))[511]=(uint64_t)KPALLOC();
	//remove bootstrap !!!
	((uint64_t*)(MBASE+0x10000))[0] = 0x0;
}
uint64_t V2P(void * V){
	uint64_t offset=(uint64_t)V&         0xFFF;
	uint64_t pt =   ((uint64_t)V&      0x1FF000) >> 12;
	uint64_t pde =((uint64_t)V&      0x3FE00000) >> 21;
	uint64_t pdpt =((uint64_t)V&   0x7FB0000000) >> 30;
	uint64_t pml4 =((uint64_t)V& 0xFF8000000000) >> 39;
	return (get_ptV(pml4,pdpt,pde,pt)&(~0xFFF))+offset;
}
void * UP_ALLOC3(uint8_t FLAGS){ //TODO: doesn't work
	uint64_t ad = PL_FN(); //here is our PHYSICAL address
	PL_SV(ad, 0x1);
	for(int pml4 = 0; pml4<1; pml4++){
		if((get_pml4V(pml4)&0x1) == 0x0){ //if our pml4 doesn't exist, we create it
			uint64_t * M = KPALLOC();
			memfill(M, 0x1000);
			((uint64_t*)(MBASE+0x10000))[pml4] = (V2P(M))|0x7; //should allocate it with kernel shit
			FLUSH_TLB();
		}
		for(int pdpt = 0; pdpt<512; pdpt++){
			if((get_pdptV(pml4, pdpt)&0x1) == 0x0){
				uint64_t * M = KPALLOC();
				memfill(M, 0x1000);
				get_pml4VP(pml4)[pdpt] = (V2P(M))|0x7; //should allocate it with kernel shit
				FLUSH_TLB();
			}
			for(int pde = 0; pde<512; pde++){
				if((get_pdeV(pml4, pdpt, pde)&0x1) == 0x0){
					uint64_t * M = KPALLOC(); //right here I believe we RUN OUT of pages, and it tries to allocate a new one with kpalloc3, which fails for some reason...
					memfill(M, 0x1000);
					get_pdptVP(pml4, pdpt)[pde] = (V2P(M))|0x7; //should allocate it with kernel shit
					FLUSH_TLB();
				}
				for(int pt = 0; pt<512; pt++){
					if((get_ptV(pml4, pdpt, pde, pt) & 0x1)==0){ //some kind of error in ptV for a usermode
										     //i THINK it's because it assumes that pml4 exists, when it doesn't !!!
						//woo hoo ! we have found our virtual address !!	
						get_pdeVP(pml4,pdpt,pde)[pt] = (0x1000*ad)|0x5|FLAGS;
						FLUSH_TLB();
						return (void*)construct_virtual_address(pml4,pdpt,pde,pt);
					}
				}
				if(pde<511){
					if((get_pdeV(pml4,pdpt,pde+1)&0x1) == 0){
						// boo hoo ! next pde doesn't exist
						uint64_t* M = KPALLOC();
						memfill(M, 0x1000);
						get_pdptVP(pml4, pdpt)[pde+1] = V2P(M)|0x27|0x7;
						M[0]=(0x1000*ad)|0x5|FLAGS;
						FLUSH_TLB();
						return (void*)construct_virtual_address(pml4,pdpt,pde+1,0x0);

					}else{
						continue;
					}
				}else{
					if(pdpt<511){
						if((get_pdptV(pml4, pdpt+1)&0x1) == 0){
							//a greater pdpt than we can contradict
							//hath unallocated our intents
							uint64_t* M = KPALLOC();
							memfill(M, 0x1000);
							uint64_t* M2 = KPALLOC();
							memfill(M2, 0x1000);
							get_pml4VP(pml4)[pdpt+1]=V2P(M)|0x27;
							M[0]=V2P(M2)|0x27;
							M2[0]=(0x1000*ad)|0x5|FLAGS;
							FLUSH_TLB();
							return (void*) construct_virtual_address(pml4,pdpt+1,0x0,0x0);
						}else{
							continue;
						}
					}else{
						if(pml4<0){
							if((get_pml4V(pml4+1)&0x1) == 0){
								uint64_t* M = KPALLOC();
								memfill(M, 0x1000);
								uint64_t* M2 = KPALLOC();
								memfill(M2, 0x1000);
								uint64_t* M3 = KPALLOC();
								memfill(M3, 0x1000);
								((uint64_t*)(MBASE+0x10000))[pml4+1]=V2P(M)|0x27;
								M[0]=V2P(M2)|0x27;
								M2[0]=V2P(M3)|0x27;
								M3[0]=(0x1000*ad)|0x5|FLAGS;
								FLUSH_TLB();
								return (void*) construct_virtual_address(pml4+1,0x0,0x0,0x0);
							}else{
								continue;
							}
						}else {
							ERROR(ERR_PAGE_NOFREESPACE, 0x0); //no free space in the page tables
							return (void*)0x0;
						}
					}
				}
			}
		}
	}
	ERROR(ERR_PAGE_ALLOC_SHOULDNOTEXIST,0x0);
	return (void*)0x0;
}
void * KP_ALLOC3(){
	PA_CALLING = 0x1;
	uint64_t ad = PL_FN(); //here is our PHYSICAL address
	PL_SV(ad, 0x1);
	for(int pml4 = 1; pml4<512; pml4++){
		if((get_pml4V(pml4)&0x1) == 0x0){ //if our pml4 doesn't exist, we create it
			uint64_t * M = KPALLOC();
			memfill(M, 0x1000);
			((uint64_t*)(MBASE+0x10000))[pml4] = (V2P(M))|0x1; //should allocate it with kernel shit
			FLUSH_TLB();
		}
		for(int pdpt = 0; pdpt<512; pdpt++){
			if((get_pdptV(pml4, pdpt)&0x1) == 0x0){
				uint64_t * M = KPALLOC();
				memfill(M, 0x1000);
				get_pml4VP(pml4)[pdpt] = (V2P(M))|0x1; //should allocate it with kernel shit
				FLUSH_TLB();
			}
			for(int pde = 0; pde<512; pde++){
				if((get_pdeV(pml4, pdpt, pde)&0x1) == 0x0){
					uint64_t * M = KPALLOC(); //right here I believe we RUN OUT of pages, and it tries to allocate a new one with kpalloc3, which fails for some reason...
					memfill(M, 0x1000);
					get_pdptVP(pml4, pdpt)[pde] = (V2P(M))|0x1; //should allocate it with kernel shit
					FLUSH_TLB();
				}
				for(int pt = 0; pt<512; pt++){
					if((get_ptV(pml4, pdpt, pde, pt) & 0x1)==0){
						//woo hoo ! we have found our virtual address !!	
						get_pdeVP(pml4,pdpt,pde)[pt] = (0x1000*ad)|0x1;
						PA_CALLING=0x0;
						FLUSH_TLB();
						return (void*)construct_virtual_address(pml4,pdpt,pde,pt);
					}
				}
				if(pde<511){
					if((get_pdeV(pml4,pdpt,pde+1)&0x1) == 0){
						// boo hoo ! next pde doesn't exist
						uint64_t* M = KPALLOC();
						memfill(M, 0x1000);
						get_pdptVP(pml4, pdpt)[pde+1] = V2P(M)|0x23;
						//set it
						M[0]=(0x1000*ad)|0x1; //address is one too high, see PL_FN
						//set the first entry to be the address
						PA_CALLING=0x0;
						FLUSH_TLB();
						return (void*)construct_virtual_address(pml4,pdpt,pde+1,0x0);

					}else{
						continue;
					}
				}else{
					if(pdpt<511){
						if((get_pdptV(pml4, pdpt+1)&0x1) == 0){
							//a greater pdpt than we can contradict
							//hath unallocated our intents
							uint64_t* M = KPALLOC();
							memfill(M, 0x1000);
							uint64_t* M2 = KPALLOC();
							memfill(M2, 0x1000);
							get_pml4VP(pml4)[pdpt+1]=V2P(M)|0x23;
							M[0]=V2P(M2)|0x23;
							M2[0]=(0x1000*ad)|0x1;
							PA_CALLING=0x0;
							FLUSH_TLB();
							return (void*) construct_virtual_address(pml4,pdpt+1,0x0,0x0);
						}else{
							continue;
						}
					}else{
						if(pml4<511){
							if((get_pml4V(pml4+1)&0x1) == 0){
								uint64_t* M = KPALLOC();
								memfill(M, 0x1000);
								uint64_t* M2 = KPALLOC();
								memfill(M2, 0x1000);
								uint64_t* M3 = KPALLOC();
								memfill(M3, 0x1000);
								((uint64_t*)(MBASE+0x10000))[pml4+1]=V2P(M)|0x23;
								M[0]=V2P(M2)|0x23;
								M2[0]=V2P(M3)|0x23;
								M3[0]=(0x1000*ad)|0x1;
								PA_CALLING=0x0;
								FLUSH_TLB();
								return (void*) construct_virtual_address(pml4+1,0x0,0x0,0x0);
							}else{
								continue;
							}
						}else {
							ERROR(ERR_PAGE_NOFREESPACE,0x0); //no free space in the page tables
							return (void*)0x0;
						}
					}
				}
			}
		}
	}
	ERROR(ERR_PAGE_ALLOC_SHOULDNOTEXIST,0x0);
	return (void*)0x0;
}

void P_FREE(void * v_add){
	uint64_t pt = (((uint64_t)v_add)&0x1FF000)>>12;
	uint64_t pde = (((uint64_t)v_add)&(0x200*0x1ff000))>>(12+9);
	uint64_t pdpt =( ((uint64_t)v_add)&(0x200*0x200*(uint64_t)0x1ff000))>>(12+9+9);
	uint64_t pml4 = (((uint64_t)v_add)&(0x200*0x200*0x200*(uint64_t)0x1ff000))>>(12+9*3);
	PL_SV(V2P(v_add)>>12, 0x0);
	memfill(v_add, 0x1000); //initialise the memory again
	get_pdeVP(pml4,pdpt,pde)[pt] = 0x0;
	/*if(pt == 0){ //this is stupid. why did i do this.
		get_pdptVP(pml4,pdpt)[pde] = 0x0; 
		
	}*/

	FLUSH_TLB();
}
void * VERIFY_USER(void * rdx){
	if((uint64_t)(rdx) > CBASE){
		ERROR(ERR_VERIFY_USER_FAIL, (uint64_t)rdx);
	       	return NULL;
	}
	return rdx; //_might_ cause PF, but never GP
}
void U_PFREEALL(){
	//loop through pdpt -> pde -> pt, free, free, free
	//everything in UM belongs to our proc because mmio isn't implemented yet
	//tables need to have the memory which allocates the TABLE freed followed by the top level entry which points to the table AS a table
	for(int pdpt = 0; pdpt<512; pdpt++){
		if((get_pdptV(0, pdpt)&0x1) != 0x0){
			for(int pde = 0; pde<512; pde++){
				if((get_pdeV(0, pdpt, pde)&0x1) != 0x0){
					for(int pt = 0; pt<512; pt++){
						P_FREE((void*)construct_virtual_address(0, pdpt, pde, pt));
					}
					//pt does this for us
				}
				P_FREE(get_pdeVP(0, pdpt, pde));
				get_pdptVP(0, pdpt)[pde] = 0x0;
			}
		}
		P_FREE(get_pdptVP(0, pdpt)); //the kernel page which allocates the pdpt table itself
		get_pml4VP(0)[pdpt] = 0; //the entry itself
	}
}
