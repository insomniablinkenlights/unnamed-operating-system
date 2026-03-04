.code64
.section .text
L:
xchgw %bx, %bx
mov $open_sr, %rdx
movb $0x11, 2(%rdx)
mov $1, %rdi
mov $0, %rsi
mov $3, %rcx
int $0x80

movq $0, a
L2:
mov $0, %rdi
mov $0, %rsi
movq a, %rdx
add $a, %rdx
add $8, %rdx
mov $0x1, %rcx
int $0x80

mov $1, %rdi
mov $0, %rsi
movq a, %rdx
add $a, %rdx
add $8, %rdx
mov $0x1, %rcx
int $0x80
addq $1, a
jmp L2
//movq $0, 0x0

open_sr:
	.string "# "
a:
	.quad 0x0
