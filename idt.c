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
	void * pagetable;
	uint64_t rip;
	uint64_t time_to_execute;
}Task;
void newIDTEntry(void *addr, void * offset, uint16_t selector, uint8_t ist, uint8_t type_attributes){
	((InterruptDescriptor64*)addr)->offset_1=((uint64_t)offset)&0xffff;
	((InterruptDescriptor64*)addr)->offset_2=((uint64_t)offset>>16)&0xffff;
	((InterruptDescriptor64*)addr)->offset_3=((uint64_t)offset>>32)&0xffffffff;
	((InterruptDescriptor64*)addr)->selector=selector;
	((InterruptDescriptor64*)addr)->ist=ist&0x7;
	((InterruptDescriptor64*)addr)->type_attributes=type_attributes;
}
#define PIC1 0x20
#define PIC2 0xa0
void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
void FAULT();
struct interrupt_frame;
void IRQ_clear_mask(uint8_t IRQline){
	uint16_t port;
	uint8_t value;
	if(IRQline < 8){
		port=PIC1+1;
	}else{
		port=PIC2+1;
		IRQline -=8;
	}
	value = inb(port) & ~(1<<IRQline);
	outb(port, value);
}
void PIC_sendEOI(uint8_t irq){
	if(irq >= 8)
		outb(PIC2, 0x20);
	outb(PIC1,0x20);
}
void IDT_ERR_H();
void IDT_DIV_ERR();
void TIMER_INTERRUPT();
void IDT_GPF();
void IDT_DF();
void IDT_81();
/*{

	asm volatile("xchgw %bx, %bx;");
	FAULT();
}*/

/*__attribute__((interrupt)) void TIMER_INTERRUPT(struct interrupt_frame* frame){
	asm volatile("xchgw %bx, %bx");
	PIC_sendEOI(0);
}*/
void constructIDT(void *addr){
#define IEH &IDT_ERR_H
#define TI  &TIMER_INTERRUPT
	newIDTEntry(addr, &IDT_DIV_ERR, 0x08, 0x0, 0x8F); //divide error
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //debug exception
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8E); //nmi interrupt
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //breakpoint
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //overflow
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //bound exceeded
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //invalid opcode
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //device not available
	addr+=16;
	newIDTEntry(addr, &IDT_DF, 0x08, 0x0, 0x8F); //double fault
	addr+=16;
	newIDTEntry(addr, &IDT_81, 0x08, 0x0, 0x8F); //coprocessor segment overrun
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //invalid tss
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //segment not present
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //stack segment fault
	addr+=16;
	newIDTEntry(addr, &IDT_GPF, 0x08, 0x0, 0x8F); //gpf
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //page fault
	addr+=16;
	newIDTEntry(addr, (void*)(0x0), 0x0, 0x0, 0x0); //reserved
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //fpu error
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //alignment check
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //machine check
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //simd exception
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //virtualisation
	addr+=16;
	newIDTEntry(addr, (IEH), 0x08, 0x0, 0x8F); //control protection exception
								 //address is now that of 0x15*0x16
	addr+=16*11;
	newIDTEntry(addr, (TI), 0x08, 0x0, 0x8E);
	
}

void PIC_Remap(){
	//send to pic1: init byte 0x11, icw2 0x20, icw3 0x04, icw4 0x01
	//send to pic2: init byte 0x11, icw2 0x28, icw3 0x02, icw4 0x01
	outb(PIC1, 0x11);

	outb(PIC1+1, 0x20);
	outb(PIC1+1, 0x04);
	outb(PIC1+1, 0x01);
	outb(PIC2, 0x11);
	outb(PIC2+1, 0x28);
	outb(PIC2+1, 2);
	outb(PIC2+1, 0x01);
	outb(PIC1+1, 0);
	outb(PIC2+1, 0);
}
#define TIMER_RATE 100
#define PIT_FRQ 1193182
#define PIT_COUNT (PIT_FRQ/TIMER_RATE)
void PIT_SETFRQ(){
	outb(0x43, 4|48);
	outb(0x40,PIT_COUNT&0xFF);
	outb(0x40,(PIT_COUNT>>8)&0xFF);
	IRQ_clear_mask(0x0);
}
