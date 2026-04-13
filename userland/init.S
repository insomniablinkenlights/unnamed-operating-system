.code64
.section .text
_start:
mov $tty_path, %rsi
mov $0x2, %rdi
mov $0x0, %rdx
int $0x80
movq %rax, (tty_path)
movq $sh_path, %rdx
mov $0x8, %rcx
mov $0x1, %rdi
mov (tty_path), %rsi
int $0x80
mov $sh_path, %rsi
mov $0x7, %rdi
mov $sh_path, %rdx
mov $pd_path, %rcx
int $0x80
movq %rax, (sh_path)
mov %rax, %rsi
mov $0xb, %rdi
movq (tty_path), %rdx
int $0x80
movq (sh_path), %rsi
mov $0xc, %rdi
int $0x80
mov $0xa, %rdi
int $0x80
mov $0x9, %rdi
int $0x80
L:
xchgw %bx, %bx
jmp L
sh_path:
	.string "/sbin/sh"
tty_path:
	.string "/dev/tty"
pd_path:
	.quad pd2_p
	.quad 0xF
pd2_p:
	.string ""
/*L:
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
*/
