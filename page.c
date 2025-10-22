#include "headers/stdint.h"
void FLUSH_TLB();
void FAULT();
uint64_t get_pml4(int pml4){
	return ((uint64_t*)(0x10000))[pml4];
}
uint64_t get_pdpt(int pml4, int pdpt){
	return ((uint64_t*)(get_pml4(pml4)&(~0xFFF)))[pdpt];
}
uint64_t get_pde(int pml4, int pdpt, int pde){
	return ((uint64_t*)(get_pdpt(pml4, pdpt)&(~0xFFF)))[pde];
}
uint64_t get_pt(int pml4, int pdpt, int pde, int pt){
	return ((uint64_t*)(get_pde(pml4, pdpt, pde)&(~0xFFF)))[pt];
}
uint64_t construct_virtual_address(int pml4, int pdpt, int pde, int pt){
	return (uint64_t)(pt*0x1000+pde*0x1000*512+pdpt*0x1000*512*512+pml4*0x1000*512*512*512);
}
uint64_t P_ALLOC2(){
	uint64_t next_address = 0x0;
	//there is one level less recursion than there should be, I think -- TODO!
	//10000[0][0][0] = 11000[0][0] = 12000[0] == 13000, so we need to get 13000[0]
	for(int pml4 = 0; pml4<512; pml4++){
	for(int pdpt = 0; pdpt<512; pdpt++){
		for(int pde = 0; pde<512; pde++){

			for(int pt = 0; pt<(pde==511?510:511); pt++){
				if(get_pt(pml4,pdpt,pde,pt)==0){

				((uint64_t*)(get_pde(pml4,pdpt,pde)&(~0xFFF)))[pt] = 3 | next_address;
				FLUSH_TLB();
				return construct_virtual_address(pml4,pdpt,pde,pt);
				} //check here if addr>nextaddr, then add 1k to ad
				next_address+=0x1000;
				  if((get_pt(pml4,pdpt,pde,pt)&(~0xFFF))>=next_address){
				  	next_address=(get_pt(pml4,pdpt,pde,pt)&(~0xFFF))+0x1000;
				  }


			}
			//no room in pt -> check if next pde exists
			if(pde <511 && (get_pde(pml4,pdpt,pde+1)&0x1)==1){
				continue;
			}

				((uint64_t*)(get_pde(pml4,pdpt,pde)&(~0xFFF)))[511] = 3 | next_address;
				next_address+=0x1000;
				//does pdpt have room?
				if(pde<511){
					//set next pdpt entry to be the pde we just allocated
					((uint64_t*)(get_pdpt(pml4,pdpt)&(~0xFFF)))[pde+1]=(next_address-0x1000)|0x23;
					//allocate a new pt and return it
					((uint64_t*)(get_pde(pml4, pdpt, pde+1)&(~0xFFF)))[0] = (next_address|0x3);
				FLUSH_TLB();
					return construct_virtual_address(pml4,pdpt,pde+1,0); 
				}else{
					//does pml4 have room? actually im not even going to check that im lazy
					//problem is we create a gap if we don't need to allocate the next pdpt
					((uint64_t*)(get_pde(pml4, pdpt, pde)&(~0xFFF)))[510] = next_address|3; // new PDPT allocated, this fucks shit up so we need to fix it...
					((uint64_t*)next_address)[0] = (next_address-0x1000)|0x23; //fuck it
					((uint64_t*)(get_pml4(pml4)&(~0xFFF)))[pdpt+1]=next_address|0x23;
					((uint64_t*)(next_address-0x1000))[0]=(next_address+0x1000)|0x3;
				FLUSH_TLB();
					return construct_virtual_address(pml4,pdpt+1,0,0);
				}

		}
	}
	}
	//loop through pml4 -> pdpt -> pde -> check pt present
	//to make a new page table, check if pde has room
		//if so, allocate it
		//otherwise, check if pdpt has room
			//either way, allocate a new pde using currentpde limit -2 and the first free space in memory
			//if so, assign next pdpt entry to our pde
				//then allocate first entry in new pde and return it
			//otherwise, allocate a new pdpt using current pde limit -1 and first free space
				//then, link pde into it and then link that into pml4
					//then allocate first entry in new pde and return it
	FAULT();
}
void P_FREE(uint64_t v_add){
	uint64_t pt = (v_add&0x1FF000)/0x1000;
	uint64_t pde = (v_add&(0x200*0x1ff000))/(0x1000*0x200);
	uint64_t pdpt =( v_add&(0x200*0x200*(uint64_t)0x1ff000))/(0x1000*0x200*0x200);
	uint64_t pml4 = (v_add&(0x200*0x200*0x200*(uint64_t)0x1ff000))/(0x1000*0x200*0x200*(uint64_t)0x200);
	((uint64_t*)(get_pde(pml4,pdpt,pde)&(~0xFFF)))[pt] = 0x0;
	if(pt == 0){
		((uint64_t*)(get_pdpt(pml4,pdpt)&(~0xFFF)))[pde] = 0x0;
		
	}
	FLUSH_TLB();
}
