.code64
.text
.globl INT0x80
INT0x80:
int $0x80
ret
.global outb
outb:
mov %di, %dx
mov %si, %ax
outb %al, %dx
ret
.global inb
inb:
xor %rax, %rax
mov %di, %dx
inb %dx, %al
ret
