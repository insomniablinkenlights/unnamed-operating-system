#ifndef addresses
#define addresses
#define page_directory_address = 0x10000;
#define CBASE 0x8000000000
#define MBASE CBASE+0x10000
void * KPALLOC();
void * UPALLOC(uint8_t FLAGS);
void FAULT();
void * malloc(uint64_t size);
void free(void * ptr);
void initMalloc();
void CLI();
void P_FREE(void * v_add);
void STI();
void HLT();
void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
void memcpy(void * dest, void * src, uint64_t count);
void memfill(void * dest, uint64_t count);
void wait_for_irq(uint8_t irq, uint64_t timeout, void * timeout_function);
void nano_sleep(uint64_t t);
void ERROR(uint64_t code, uint64_t type);
enum ERROR_CODES{
	ERR_FSC,
	ERR_FSC2,
	ERR_FSC3,
	ERR_FSC4,
	ERR_FSCUNIMP,
	ERR_WFIRQ,
	ERR_FVNS,
	ERR_FRLX,
	ERR_FDC_INUSE,
	ERR_INODE_NE,
	ERR_FILE_PASTBOUND,
	ERR_READ_SIZE_TEMP,
	ERR_FILE_DIRSTREAM,
	ERR_IN_DIR_DNE,
	ERR_IN_FILE_DNE,
	ERR_FD_TOOHIGH,
	ERR_FD_DNE,
	ERR_TT_NT,
};
#define NULL  ((void*)0x0)

#endif
