.code64
.section .text
.global outb;
.global FLUSH_TLB;
.global FAULT;
outb:
push %rdx
push %rax
mov %di, %dx
mov %si, %ax
outb %al, %dx
pop %rax
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
