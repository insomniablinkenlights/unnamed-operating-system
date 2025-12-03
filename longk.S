.code64
.section .text
.global _M64;
_M64:
/*
paging setup
four level tree, first node 0x10000
we need a system to check if a node is taken
page format?
*/
mov $0x8000007bff, %rsp
mov $0x8000007bff, %rbp
mov $0x80000008, %eax
//cpuid
and $0xFF00, %eax
shr $8, %eax
mov $0x10, %bx
mov %bx, %ss

//set PIC to signal int 0x0-0x7 for 0x20-0x27 as to not map to CPU bs
//call PIC_Remap

//call P_ALLOC
//movq $0x300003, 0x14000
//movq $0x14023, 0x12008
xchgw %bx, %bx
call P_ALLOC3_SETUP
call KPALLOC



//call P_ALLOC2
//eax contains a pointer to page info

mov %rax, %rdx
call PCI_SCAN
//mov %rax, %rdi

//call P_FREE
//call P_ALLOC2
call KPALLOC
mov %rax, %rdi
push %rax
call constructIDT
pop %rax
cli
movabsq $idtr_64, %r9
movq $(0x21*16), (%r9)
movq %rax, 2(%r9)
lidt (%r9)
call PIC_Remap
call PIT_SETFRQ
cli
//sti

//ISA FLOPPY
/*mov $0x70, %dx
mov $(1<<7 | 0x10), %al
xchgw %bx, %bx
outb %al, %dx
inb $0x71, %al*/

//test ISA
//we have a privileged isa pci device so it should emulate it without dma stuff
//outb $0x0, $0x279
//outb $0x0, $0x279
//outb $0x40, $0xA79
/*OUTL:
mov $0x0, %al
mov $0x279, %dx
outb %al, %dx
mov $0x5, %al
outb %al, %dx
mov $0x203, %dx
inb %dx, %al
and $1, %al
cmp $0, %al
je OUTL
mov $0x279, %dx
mov $0x0, %al
outb %al, %dx
mov $0x4, %al
outb %al, %dx
mov $0x203, %dx
inb %dx, %al
*/
	//TODO
cli
call initialize_multitasking
movabsq $L2, %rdi
call create_kernel_task
xchgw %bx, %bx
sti
//rax: L2
L:
//push %rax
//mov %rax, %rdi
//call switch_to_task
nop
mov $1000000000, %rdi
call nano_sleep
xchgw %bx, %bx
//pop %rax
jmp L
L2:
xchgw %bx, %bx
mov $1000000000, %rdi
call nano_sleep
//push %rax
//movabsq $(0x8000000000+0x1d000), %rdi
//call switch_to_task
//pop %rax
jmp L2
idtr_64:
.byte 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0

