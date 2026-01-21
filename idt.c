#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/idt.h"

typedef struct InterruptDescriptor64 {
	uint16_t offset_1; //offset bits 0-15
	uint16_t selector; //code segment selector in gdt or ldt
	uint8_t ist; //bits 0-2 are interrupt stack table offset, rest are zero
	uint8_t type_attributes; //gate type, dpl, and p fields
	uint16_t offset_2; //offset 16-31
	uint32_t offset_3; //offset 32-63
	uint32_t zero; //reserved
}InterruptDescriptor64;
void newIDTEntry(void *addr, void offset(void), uint16_t selector, uint8_t ist, uint8_t type_attributes, uint64_t offset2){
	addr=(char*)(addr)+(offset2<<4);
	((InterruptDescriptor64*)addr)->offset_1=((uint64_t)offset)&0xffff;
	((InterruptDescriptor64*)addr)->offset_2=((uint64_t)offset>>16)&0xffff;
	((InterruptDescriptor64*)addr)->offset_3=((uint64_t)offset>>32)&0xffffffff;
	((InterruptDescriptor64*)addr)->selector=selector;
	((InterruptDescriptor64*)addr)->ist=ist&0x7;
	((InterruptDescriptor64*)addr)->type_attributes=type_attributes;
}
#define PIC1 0x20
#define PIC2 0xa0
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
/*{

	asm volatile("xchgw %bx, %bx;");
	FAULT();
}*/

/*__attribute__((interrupt)) void TIMER_INTERRUPT(struct interrupt_frame* frame){
	asm volatile("xchgw %bx, %bx");
	PIC_sendEOI(0);
}*/

void constructIDT(void *addr){
#define TI  &TIMER_INTERRUPT
	newIDTEntry(addr, &IDT_DE, 0x08, 0x0, 0x8F, 0); //divide error
	newIDTEntry(addr, &IDT_DB, 0x08, 0x0, 0x8F, 1); //debug exception
	newIDTEntry(addr, &IDT_NMI, 0x08, 0x0, 0x8E, 2); //nmi interrupt
	newIDTEntry(addr, &IDT_BP, 0x08, 0x0, 0x8F, 3); //breakpoint
	newIDTEntry(addr, &IDT_OF, 0x08, 0x0, 0x8F, 4); //overflow
	newIDTEntry(addr, &IDT_BR, 0x08, 0x0, 0x8F,5 ); //bound exceeded
	newIDTEntry(addr, &IDT_UD, 0x08, 0x0, 0x8F,6); //invalid opcode
	newIDTEntry(addr, &IDT_NM, 0x08, 0x0, 0x8F,7); //device not available
	newIDTEntry(addr, &IDT_DF, 0x08, 0x0, 0x8F,8); //double fault
	newIDTEntry(addr, NULL, 0x08, 0x0, 0x8F,9); //coprocessor segment overrun
	newIDTEntry(addr, &IDT_TS, 0x08, 0x0, 0x8F,10); //invalid tss
	newIDTEntry(addr, &IDT_NP, 0x08, 0x0, 0x8F,11); //segment not present
	newIDTEntry(addr, &IDT_SS, 0x08, 0x0, 0x8F,12); //stack segment fault
	newIDTEntry(addr, &IDT_GP, 0x08, 0x0, 0x8F,13); //gpf
	newIDTEntry(addr, &IDT_PF, 0x08, 0x0, 0x8F,14); //page fault
	newIDTEntry(addr, NULL, 0x0, 0x0, 0x0,15); //reserved
	newIDTEntry(addr, &IDT_MF, 0x08, 0x0, 0x8F,16); //fpu error
	newIDTEntry(addr, &IDT_AC, 0x08, 0x0, 0x8F,17); //alignment check
	newIDTEntry(addr, &IDT_MC, 0x08, 0x0, 0x8F,18); //machine check
	newIDTEntry(addr, &IDT_XM, 0x08, 0x0, 0x8F,19); //simd exception
	newIDTEntry(addr, &IDT_VE, 0x08, 0x0, 0x8F,20); //virtualisation
	newIDTEntry(addr, &IDT_CP, 0x08, 0x0, 0x8F,21); //control protection exception
	newIDTEntry(addr, (TI), 0x08, 0x0, 0x8E, 0x20);
	newIDTEntry(addr, (PS2_HANDLER), 0x08, 0x0, 0x8E, 0x21); //0x21, int 1
	newIDTEntry(addr, NULL, 0x0, 0x0, 0x0, 0x22); //0x22, int 2
	newIDTEntry(addr, NULL, 0x0, 0x0, 0x0, 0x23); //0x23, int 3
	newIDTEntry(addr, NULL, 0x0, 0x0, 0x0, 0x24); //0x24, int 4
	newIDTEntry(addr, NULL, 0x0, 0x0, 0x0, 0x25); //0x25, int 5
	newIDTEntry(addr, (FL), 0x8, 0x0, 0x8E, 0x26); //0x26, int 6
	newIDTEntry(addr, (SPIRQ), 0x8, 0x0, 0x8F, 0x27); //0x27, int 7
	newIDTEntry(addr, (PS2_HANDLER12), 0x8, 0x0, 0x8E, 0x2c); //0x2c, int 12
	newIDTEntry(addr, Int0x80Handler, 0x8, 0x0, 0x8E|0b01100000, 0x80); //0x80, int 0x80	
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
void PIC_FL(){
	IRQ_clear_mask(0x6);
}
void PIC_PS2(){
	IRQ_clear_mask(0x1);
	IRQ_clear_mask(12);
}
