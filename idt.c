#include "headers/stdint.h"

typedef struct InterruptDescriptor64 {
	uint16_t offset_1; //offset bits 0-15
	uint16_t selector; //code segment selector in gdt or ldt
	uint8_t ist; //bits 0-2 are interrupt stack table offset, rest are zero
	uint8_t type_attributes; //gate type, dpl, and p fields
	uint16_t offset_2; //offset 16-31
	uint32_t offset_3; //offset 32-63
	uint32_t zero; //reserved
}InterruptDescriptor64;
typedef struct Task{
	uint8_t ring;
}Task;
void newIDTEntry(void *addr, void * offset, uint16_t selector, uint8_t ist, uint8_t type_attributes){
	((InterruptDescriptor64*)addr)->offset_1=(uint64_t)offset&0xffff;
	((InterruptDescriptor64*)addr)->offset_2=((uint64_t)offset>>16)&0xffff;
	((InterruptDescriptor64*)addr)->offset_3=((uint64_t)offset>>32)&0xffffffff;
	((InterruptDescriptor64*)addr)->selector=selector;
	((InterruptDescriptor64*)addr)->ist=ist&0x7;
	((InterruptDescriptor64*)addr)->type_attributes=type_attributes;
}
#define PIC1 0x20
#define PIC2 0xa0
void outb(uint16_t port, uint8_t data);
void PIC_sendEOI(uint8_t irq){
	if(irq >= 8)
		outb(PIC2, 0x20);
	outb(PIC1,0x20);
}
void IDT_ERR_H(){
}
void TIMER_INTERRUPT(){
	PIC_sendEOI(0);
}
void constructIDT(void *addr){
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //divide error
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //debug exception
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8E); //nmi interrupt
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //breakpoint
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //overflow
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //bound exceeded
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //invalid opcode
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //device not available
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //double fault
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //coprocessor segment overrun
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //invalid tss
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //segment not present
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //stack segment fault
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //gpf
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //page fault
	addr+=16;
	newIDTEntry(addr, 0x0, 0x0, 0x0, 0x0); //reserved
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //fpu error
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //alignment check
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //machine check
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //simd exception
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //virtualisation
	addr+=16;
	newIDTEntry(addr, *(IDT_ERR_H)+0x7c00, 0x08, 0x0, 0x8F); //control protection exception
								 //address is now that of 0x15*0x16
	addr+=16*9;
	newIDTEntry(addr, *(TIMER_INTERRUPT)+0x7c00, 0x08, 0x0, 0x8E);
	
}

void PIC_Remap(){
	outb(PIC1, 0x11);

	outb(PIC2, 0x11);
	outb(PIC1+1, 0x20);
	outb(PIC2+1, 0x28);
	outb(PIC1+1, 1<<2);
	outb(PIC2+1, 2);
	outb(PIC1+1, 0x01);
	outb(PIC2+1, 0x01);
	outb(PIC1+1, 0);
	outb(PIC2+1, 0);
}
#define TIMER_RATE 100
#define PIT_FRQ 1193182
#define PIT_COUNT (PIT_FRQ/TIMER_RATE)
void PIT_SETFRQ(){
	outb(0x43, 0x0);
}
