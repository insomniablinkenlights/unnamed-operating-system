#include "headers/stdint.h"
#include "headers/addresses.h"
#define PTV_MEMORYOVERHEAD 0
void FLUSH_TLB();


uint64_t * manualptVlookup(uint64_t AD){ //TODO: measure cpu usage
	//USE: 0x1c000
	*((uint64_t*)(MBASE+0x13160)) = AD|0x3;	//this needs to be changed whenever mbase is
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
}uint64_t * get_pdptVP(int pml4, int pdpt){ //return value: pointer to first guy in pde
	return manualptVlookup(get_pdptV(pml4, pdpt)&(~0xFFF));
}uint64_t * get_pml4VP(int pml4){ //return value: pointer to first guy in pde
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
	FAULT(); // unimplemented -- illusion of choice :3
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
		FAULT();
	}
	if(AN >= 32){
		if(!PA_CALLING){
			for(int i = 0; i<32; i++){
				if(((uint64_t*)(MBASE+0x16000))[i]!=0x0) continue;
				((uint64_t*)(MBASE+0x16000))[i] = (uint64_t)KP_ALLOC3();
			}
		}
	}
	return (void*)k;
}
void * UPALLOC(uint8_t FLAGS){ // THIS FUNCTION BREAKS IF CALLED TWICE
	//ACTUALLY, fuck all this, temporary bullshit go !!!
	return UP_ALLOC3(FLAGS);
	
	
	// 0xc0017000 contains a stack of 64 bit page virtual addresses
	// doesn't work
	// 0xc0017000 contains a stack of virtual pointers to PTs
	// we modify the flags of the PT, convert its ph.ad to vt, return
/*	uint64_t k = 0x0;
	uint16_t AN = 0x0;
	for(int i = 0; i<512; i++){
		AN = i;
		if(((uint64_t*)0xc0017000)[i] != 0x0){
			k = ((uint64_t*)0xc0017000)[i];
			((uint64_t*)0xc0017000)[i] = 0x0;
			break;
		}
	}
	if(k == 0x0){
		FAULT();
	}
	if(AN > 500){
		if(!PA_CALLING){
			for(int i = 0; i<511; i++){
				if(((uint64_t*)0xc0017000)[i]==0x0) continue;
				((uint64_t*)0xc0017000)[i] = (uint64_t)UP_ALLOC3(0x0); //breaks
			}
		}
	}

	return (void*)k; //breaks*/
}
void * PL_MAP_START = (void*)(MBASE+0x15000);
void * LL_NV(uint64_t index, void * PLM){
	if(index > 510){
		if(((uint64_t*)PLM)[511] == 0x0){
			((uint64_t*)PLM)[511] = (uint64_t)KPALLOC();
		}
		return LL_NV(index-511, (void*)(((uint64_t*)PLM)[511]));
	}
	return PLM+(8*index);
}
void PL_SV(uint64_t index, uint8_t val){
	*(uint64_t*)LL_NV(index/64, PL_MAP_START) |= val << (index%64); //TODO: doesn't deallocate
}
uint64_t PL_FN(){
	int i = 0;
	uint64_t * PT = PL_MAP_START;
	while(1){
		for(int j = 0; j<511; j++){
			if(PT[j] != 0xFFFFFFFFFFFFFFFF){
			//	while(!((PT[j]>>((i++)%64))&0x1)); return i;
				return i + __builtin_ctzll(~PT[j]);
			}
			i+=64;
		}
		if(PT[511]==0x0){
			PT[511]=(uint64_t)KPALLOC();
		}
		PT=(uint64_t*)(PT[511]);
	}
}
void P_ALLOC3_SETUP(){
	PA_CALLING=0x0;
	for(int i = 0; i<512; i++){
		PL_SV(i, 1);
	}
	((uint64_t*)(MBASE+0x16000))[501]=MBASE+0x19000; // pde list plus pdpt list is two extra pages to start
	((uint64_t*)(MBASE+0x16000))[502]=MBASE+0x1A000;
	((uint64_t*)(MBASE+0x16000))[503]=MBASE+0x1E000;
	((uint64_t*)(MBASE+0x17000))[502]=MBASE+0x1B000;

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
		for(int pdpt = 0; pdpt<512; pdpt++){
			for(int pde = 0; pde<512; pde++){
				for(int pt = 0; pt<512; pt++){
					if((get_ptV(pml4, pdpt, pde, pt) & 0x1)==0){
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
						get_pdptVP(pml4, pdpt)[pde+1] = V2P(M)|0x27;
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
							uint64_t* M2 = KPALLOC();
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
								uint64_t* M2 = KPALLOC();
								uint64_t* M3 = KPALLOC();
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
							FAULT(); //no free space in the page tables
							return (void*)0x0;
						}
					}
				}
			}
		}
	}
	FAULT();
	return (void*)0x0;
}
void * KP_ALLOC3(){
	PA_CALLING = 0x1;
	uint64_t ad = PL_FN(); //here is our PHYSICAL address
	PL_SV(ad, 0x1);
	for(int pml4 = 1; pml4<512; pml4++){
		for(int pdpt = 0; pdpt<512; pdpt++){
			for(int pde = 0; pde<512; pde++){
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
						get_pdptVP(pml4, pdpt)[pde+1] = V2P(M)|0x23;
						M[0]=(0x1000*ad)|0x1;
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
							uint64_t* M2 = KPALLOC();
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
								uint64_t* M2 = KPALLOC();
								uint64_t* M3 = KPALLOC();
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
							FAULT(); //no free space in the page tables
							return (void*)0x0;
						}
					}
				}
			}
		}
	}
	FAULT();
	return (void*)0x0;
}

void P_FREE(uint64_t v_add){
	uint64_t pt = (v_add&0x1FF000)/0x1000;
	uint64_t pde = (v_add&(0x200*0x1ff000))/(0x1000*0x200);
	uint64_t pdpt =( v_add&(0x200*0x200*(uint64_t)0x1ff000))/(0x1000*0x200*0x200);
	uint64_t pml4 = (v_add&(0x200*0x200*0x200*(uint64_t)0x1ff000))/(0x1000*0x200*0x200*(uint64_t)0x200);
	get_pdeVP(pml4,pdpt,pde)[pt] = 0x0;
	if(pt == 0){
		get_pdptVP(pml4,pdpt)[pde] = 0x0;
		
	}
	FLUSH_TLB();
}
