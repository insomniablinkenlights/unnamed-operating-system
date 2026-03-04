#ifndef elf
#define elf
#include "stdint.h"
#define Elf64_Half uint16_t
#define Elf64_Word uint32_t
#define Elf64_Sword int32_t
#define Elf64_Xword uint64_t
#define Elf64_Addr uint64_t
#define Elf64_Off uint64_t
#define Elf64_Sxword int64_t
typedef struct {
	uint8_t e_ident[16];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
}Elf64_Ehdr;
enum Elf_Ident {
	EI_MAG0=0, //0x7F
	EI_MAG1 = 1,
	EI_MAG2=2,
	EI_MAG3, EI_CLASS, EI_DATA, EI_VERSION, EI_OSABI, EI_ABIVERSION, EI_PAD
};
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFDATA2LSB (1)
#define ELFCLASS64 (2)
enum Elf_Type {
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2
};
#define EM_386 (3)
#define EM_X86_64 (62)
#define EV_CURRENT (1)
typedef struct {
	Elf64_Word sh_name; //offset of a string in the section name string table
			    //the table itself is defined by e_shstrndx
	Elf64_Word sh_type;
	Elf64_Word sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Word sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Word sh_addralign;
	Elf64_Word sh_entsize;
}Elf64_Shdr;
#define SHN_UNDEF (0x00)
#define SHN_ABS 0xFFF1
enum ShT_Types {
	SHT_NULL = 0,
	SHT_PROGBITS=1,
	SHT_SYMTAB=2,
	SHT_STRTAB=3,
	SHT_RELA=4,
	SHT_NOBITS=8,
	SHT_REL=9,
};
enum ShT_Attributes {
	SHF_WRITE=0x01,
	SHF_ALLOC=0x02,
};
typedef struct {
	Elf64_Word st_name;
	uint8_t st_info;
	uint8_t st_other;
	Elf64_Half st-shndx;
	Elf64_Addr st_value;
	Elf64_Xword st_size;
}Elf64_Sym;
#define ELF64_ST_BIND(INFO) ((INFO)>>4)
#define ELF64_ST_TYPE(INFO) ((INFO)&0x0F)
enum StT_Bindings {
	STB_LOCAL = 0,
	STB_GLOBAL = 1,
	STB_WEAK = 2
};
enum StT_Types {
	STT_NOTYPE = 0,
	STT_OBJECT=1,
	STT_FUNC = 2
};
//relocation entries: one without an explicit, one with.
typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
}Elf64_Rel;
typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
}Elf64_Rela;
#define ELF64_R_SYM(INFO) ((INFO)>>8)
#define ELF64_R_TYPE(INFO) ((uint8_t)(INFO))
enum RtT_Types {
	R_X86_64_NONE=0,
	R_X86_64_64=1,
	R_X86_64_PC64=24
};
#endif
