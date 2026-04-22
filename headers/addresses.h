#include "stdint.h"
#include "error_types.h"
#ifndef addresses
#define addresses
#define page_directory_address = 0x10000;
#define CBASE 0x8000000000
#define MBASE (CBASE+0x20000)
void * KPALLOC();
void * KPALLOCS(int64_t size);
void * UPALLOC(uint8_t FLAGS, void * initial, int64_t size_pages);
void FAULT();
void * malloc(uint64_t size);
void free(void * ptr);
void initMalloc();
void CLI();
void P_FREE(void * v_add);
void P_FREES(void * v_add, int64_t l);
void STI();
void BREAK(uint64_t data);
void HLT();
void PIC_sendEOI(uint8_t irq);
void PIC_PS2();
void PIC_PS2off();
void COM_IRQ();
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
void lIOPL(uint64_t no);
void ERROR(uint64_t code, uint64_t type);
#endif
