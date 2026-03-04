#include "headers/elf.h"
#include "headers/stdint.h"
#include "headers/addresses.h"
uint8_t elf_check_file (Elf64_Ehdr *hdr){
	if(!hdr) return 0;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1 || hdr->e_ident[EI_MAG2] != ELFMAG2 || hdr->e_ident[EI_MAG3] != ELFMAG3){
		ERROR(ERR_ELF_MAGIC, (uint64_t)hdr);
		return 0;
	}
	return 1;
} 
bool elf_check_supported(Elf64_Ehdr *hdr){
	if(!elf_check_file(hdr)){
		return 0;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS64 || hdr->e_ident[EI_DATA] != ELFDATA2LSB || hdr->e_machine != EM_X86_64 || hdr->e_ident[EI_VERSION] != EV_CURRENT){
		return 0;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC){
		return 0;
	}
	return 1;
}
static inline Elf64_Shdr *elf_sheader(Elf64_Ehdr *hdr){
	return (Elf64_Shdr*)((uint64_t)hdr + hdr->e_shoff);
}
static inline Elf64_Shdr *elf_section(Elf64_Ehdr *hdr, uint64_t idx){
	return &elf_sheader(hdr)[idx];
}
//section names
static inline char*elf_str_table(Elf64_Ehdr * hdr){
	if(hdr->e_shstrndx == SHN_UNDEF) return NULL;
	return (char*)hdr+elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}
static inline char*elf_lookup_string(Elf64_Ehdr *hdr, uint64_t offset){
	char * strtab = elf_str_table(hdr);
	if(strtab == NULL) return NULL;
	return strtab + offset;
}
void * elf_lookup_symbol(const char* name){
	return NULL; //TODO: lookup a symbol definition by name...
}
//compute absolute address of the value of the symbol, used often for linking and reloc
static uint64_t elf_get_symval(Elf64_Ehdr *hdr, uint64_t table, uint64_t idx){
	if(table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
	Elf64_Shdr * symtab = elf_section(hdr, table);
	uint32_t symtab_entries = DIV64_32(symtab->sh_size, symtab->sh_entsize);
	if(idx >= symtab_entries){
		ERROR(ERR_SYMI_OOR, idx);
	}
	uint64_t symaddr = (uint64_t)hdr+ symtab->sh_offset;
	Elf64_Sym *symbol = &((Elf64_Sym *)symaddr)[idx];
	//the above checks against the symbol table index and the symbol index
	if(symbol ->st_shndx == SHN_UNDEF){
		//external symbol, lookup value
		Elf64_Shdr *strtab = elf_section(hdr, symtab->sh_link);
		const char *name = (const char*)hdr+symtab->sh_offset+symbol->st_name;
		void *target = elf_lookup_symbol(name); //lookup a symbol definition by name and return an absolute address
		if(target == NULL){
			//external symbol not found
			if(ELF64_ST_BIND(symbol->st_info)&STB_WEAK){
				//weak symbol initialised as 0
				return 0;
			}else{
				ERROR(ERR_UNDEF_EXTSYM, (uint64_t)name);
				return 0;
			}
		}else{
			return (uint64_t)target;
		}
	}else if(symbol->st_shndx == SHN_ABS){
		//absolute symbol
		return symbol->st_value;
	}else{
		//internally defined symbol
		Elf64_Shdr *target = elf_section(hdr, symbol->shndx);
		return (uint64_t)hdr + symbol->st_value + target->sh_offset;
	}
			
}
static int elf_load_stage1(Elf64_Ehdr *hdr){ //load the section BSS, or any others with SHT_NOBITS. should be done before any operation such as relocation.
	Elf64_Shdr *shdr = elf_sheader(hdr);
	uint64_t i;
	for(i = 0; i<hdr->e_shnum; i++){
		Elf64_Shdr *section = &shdr[i];
		if(section->sh_type == SHT_NOBITS){
			if(!section->sh_size)continue; //skip it if the section is empty
			if(section->sh_flags & SHF_ALLOC){
				if(section->sh_size & 0xFFF){
					ERROR(ERR_BSS_INVALID_SIZE, section->sh_size);
				}
				void * mem = KPALLOCS((section->sh_size)>>12);
				memfill(mem, section->sh_size);
				//assign the memory offset to the section offset
				section->sh_offset = (uint64_t)mem - (uint64_t)hdr;
			}
		}
	}
	return 0;
}
static int elf_load_stage2(Elf64_Ehdr *hdr){
	Elf64_Shdr *shdr = elf_sheader(hdr);
	uint64_t i, idx;
	//iterate over sec headers
	for(i = 0; i<hdr->e_shnum; i++){
		Elf64_Shdr *section = &shdr[i];
		//if relocation section
		if(section->sh_type == SHT_REL){
			for(idx = 0; DIV64_32(idx<section->sh_size, section->sh_entsize); idx++){
				Elf64_Rel *reltab = &((Elf64_Rel *)((uint64_t)hdr+section->sh_offset))[idx];
				int result = elf_do_reloc(hdr, reltab, section);
				if(result == -1){
					ERROR(ERR_RLC_FAIL, (uint64_t)hdr);
				}
			}
		}
	}
	return 0;
}
static int elf_do_reloc(Elf64_Ehdr *hdr, Elf64_Rel *rel, Elf64_Shdr *reltab){
	Elf64_Shdr *target = elf_section(hdr, reltab->sh_info);
	uint64_t addr = (uint64_t)hdr+target->sh_offset;
	uint64_t *ref = (uint64_t*)(addr+rel->r_offset);
	//addr is start of symbol's section, ref is reference to symbol
	uint64_t symval = 0;
	if(ELF64_R_SYM(rel->r_info) != SHN_UNDEF){
		symval = elf_get_symval(hdr, reltab->sh_link, Elf64_R_SYM(rel->r_info));
		if(symval == -1) return -1;
	}
	//relocate based on type ^w^
	switch(ELF64_R_TYPE(rel->r_info)){
		case R_X86_64_NONE:
			break;
		case R_X86_64_64:
			*ref = symval+*ref;
			break;
		case R_X86_64_PC64:
			*ref = symval+*ref-(uint64_t)ref;
			break;
		default:
			ERROR(ERR_RLC_UNSP, ELF64_R_TYPE(rel->r_info));
			return -1;
	}
	return symval;
}
static inline void *elf_load_rel(Elf64_Ehdr *hdr){
	elf_load_stage1(hdr);
	elf_load_stage2(hdr);
	//TODO: parse the program header (if present)
	return (void*)hdr->e_entry;
}
void * elf_load_file(void * file){
	Elf64_Ehdr *hdr = (Elf64_Ehdr *)file;
	if(!elf_check_supported(hdr)){
		ERROR(ERR_ELF_UNSUP, (uint64_t)hdr);
	}
	switch(hdr->e_type){
		case ET_EXEC:
			//TODO: implement
			return NULL;
		case ET_REL:
			return elf_load_rel(hdr);
	}
	return NULL;
}
