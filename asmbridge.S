.code64
.section .text
.global outb;
outb:
push %rdx
push %rax
mov %di, %dx
mov %si, %ax
outb %al, %dx
pop %rax
pop %rdx
ret
