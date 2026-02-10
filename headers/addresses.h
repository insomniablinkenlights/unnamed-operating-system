#ifndef addresses
#define addresses
#define page_directory_address = 0x10000;
#define CBASE 0x8000000000
#define MBASE (CBASE+0x20000)
#define MIN(a,b) (((a)<(b))?(a):(b))
void * KPALLOC();
void * UPALLOC(uint8_t FLAGS);
void FAULT();
void * malloc(uint64_t size);
void free(void * ptr);
void initMalloc();
void CLI();
void P_FREE(void * v_add);
void STI();
void BREAK(uint64_t data);
void HLT();
void PIC_sendEOI(uint8_t irq);
void PIC_PS2();
void PIC_PS2off();
void U_PFREEALL();
int checkCorruption();
uint32_t DIV64_32(uint64_t dd, uint32_t ds);
uint32_t MOD64_32(uint64_t dd, uint32_t ds);
void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
void memcpy(void * dest, void * src, uint64_t count);
void memfill(void * dest, uint64_t count);
void wait_for_irq(uint8_t irq, uint64_t timeout, void timeout_function(void));
void nano_sleep(uint64_t t);
void ERROR(uint64_t code, uint64_t type);
enum ERROR_CODES{
	ERR_FSC = 0,
	ERR_FSC2 = 1,
	ERR_FSC3 = 2,
	ERR_FSC4 = 3,
	ERR_FSCUNIMP = 4,
	ERR_WFIRQ = 5,
	ERR_FVNS = 6,
	ERR_FRLX = 7,
	ERR_FDC_INUSE = 8,
	ERR_INODE_NE = 9,
	ERR_FILE_PASTBOUND = 0xa,
	ERR_READ_SIZE_TEMP = 0xb,
	ERR_FILE_DIRSTREAM = 0xc,
	ERR_IN_DIR_DNE = 0xd,
	ERR_IN_FILE_DNE = 0xe, //it's entirely my fault that this lines up with PF
	ERR_FD_TOOHIGH = 0xf,
	ERR_FD_DNE = 0x10,
	ERR_TT_NT = 0x11,
	ERR_FREE_BADMETADATA = 0x12,
	ERR_FREE_SIZEMISMATCH = 0x13,
	ERR_PIT_WFI_MISP = 0x14,
	ERR_DOUBLE_FREE = 0x15,
	ERR_FREE_INVALID_PTR=0x16,
	ERR_FREE_INVALID_PTR2=0x17,
	ERR_FWDL = 0x18,
	ERR_WRITE_DISKINV = 0x19,
	ERR_READ_DISKINV = 0x1a,
	ERR_FILESTREAM_RW=0x1b,
	ERR_FDT_MISMATCH=0x1c,
	ERR_FDT_W_MISMATCH=0x1d,
	ERR_MALLOC_OVERALLOC=0x1e,
	ERR_BROKEN_MEMCPY=0x1f,
	ERR_PAGE_NOFREESPACE=0x20,
	ERR_PAGE_ALLOC_SHOULDNOTEXIST=0x21,
	ERR_KPALLOC_NOFREE=0x22,
	ERR_PL_FN_MISMATCH=0x23,
	ERR_READ_SIZE=0x24,
	ERR_WRITE_SIZE=0x25,
	ERR_NOT_UM=0x26,
	ERR_FILE_OPEN_SLASHCOUNT=0x27,
	ERR_FILE_OPEN_WHY=0x28,
	ERR_MALLOC_SIZE0=0x29,
	ERR_MALLOC_SIZEMAX=0x2a,
	ERR_VERIFY_USER_FAIL=0x2b,
	ERR_8042_SELF_TEST=0x2c,
	ERR_PS2_NO_CHANNELS=0x2d,
	ERR_CTTCBN = 0x2e,
	ERR_PS2_DATA_TIMEOUT=0x2f,
	ERR_PS2_READ_TIMEOUT=0x30,
	ERR_PS2_CMD_TIMEOUT=0x31,
	ERR_TFNTTNNZ = 0x32,
	ERR_SPLIT_FAIL = 0x33,
	ERR_OPEN_TWO=0x34,
	ERR_NOKEYBOARD=0x35,
	ERR_KC_OOR=0x36,
	ERR_STDIO_NONZ=0x37,
	ERR_MBASE_TOOHIGH=0x38,
	ERR_PML4_DNE=0x39,
	ERR_PL_CLEANUP=0x3a,
	ERR_DEADCODE=0x3b,
	ERR_MISP_BINDT=0x3c,
	ERR_MISP_BINDH=0x3d,
	ERR_INT=0x3e,
};
#define NULL  ((void*)0x0)

#endif
