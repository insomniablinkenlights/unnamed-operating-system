#ifndef addresses
#define addresses
#define page_directory_address = 0x10000;
#define CBASE 0x8000000000
#define MBASE CBASE+0x10000
void * KPALLOC();
void FAULT();
void * malloc(uint64_t size);
void initMalloc();
void CLI();
void STI();
void HLT();
void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
void* memcpy(void * dest, void * src, uint64_t count);
#define NULL  ((void*)0x0)

#endif
