.code64
.section .text
.global outb;
.global inb;
.global FLUSH_TLB;
.global FAULT;
.global IDT_ERR_H;
.global TIMER_INTERRUPT;
.global IDT_DIV_ERR;
.global IDT_GPF;
.global IDT_DF;
.global IDT_81;
outb:
push %rdx
push %rax
mov %di, %dx
mov %si, %ax
outb %al, %dx
pop %rax
pop %rdx
ret
inb:
push %rdx
xor %rax, %rax
mov %di, %dx
inb %dx, %al
pop %rdx
ret
FLUSH_TLB:
push %rax
mov %cr3, %rax
mov %rax, %cr3
pop %rax
ret
FAULT:
xchgw %bx, %bx
nop
jmp FAULT

IDT_ERR_H:
xchgw %bx, %bx
mov $1, %bx
jmp FAULT
nop
iretq

TIMER_INTERRUPT:
xchgw %bx, %bx
push %rdi
xor %rdi, %rdi
call PIC_sendEOI
pop %rdi
iretq

IDT_DIV_ERR:
xchgw %bx, %bx
mov $2, %bx
jmp FAULT
iretq

IDT_GPF:
xchgw %bx, %bx
mov $3, %bx
jmp FAULT
iretq
IDT_DF:
xchgw %bx, %bx
pop %rbx
mov $0x8, %bx
jmp FAULT
iretq
IDT_81:
xchgw %bx, %bx
mov $4, %bx
jmp FAULT
iretq
