.code64
.section .text
L:
mov $open_sr, %rdx
mov $1, %rdi
mov $0, %rsi
mov $2, %rcx
int $0x80
movq $0, 0x0
L2:
//READ(0, *(0x0)+0x200, 1)
mov $0, %rsi
mov $0, %rdi
mov 0x0, %rdx
add $0x200, %rdx
mov $1, %rcx
int $0x80
//read 1 byte from stdin to 0x200
//WRITE(0x0, *(0x0)+0x200, 1)
mov $1, %rdi
mov $0, %rsi
mov 0x0, %rdx
add $0x200, %rdx
mov $1, %rcx
int $0x80
//write that to stdout
addq $1, 0x0
mov 0x0, %rdx
add $0x200, %rdx
cmpb $'\n', (%rdx)
jne L2
movb $0x0, (%rdx)
//%rdx is the end of the string now
mov $0x200, %rsi
mov $0x0, %rdx
mov $0x2, %rdi
int $0x80
mov %rax, 0x0
//open the file
//and read it to memory
//get length:
mov %rax, %rsi
mov $0, %rdx
mov $2, %rcx
mov $0x4, %rdi
int $0x80
mov 0x0, %rsi
int $0x6
shr $0x9, %rax
mov %rax, 0x8
movq $0, 0x16
L3:
mov 0x16, %rax
mov 0x8, %rbx
cmp %rax, %rbx
je L3D
mov %rax, %rcx
shl $0x9, %rcx
add $0x200, %rcx
and $0xfff, %rcx
cmp $0, %rcx
jne AAL
mov $5, %rdi
int $0x80
AAL:
mov 0x16, %rax
mov %rax, %rcx
shl $0x9, %rcx
add $0x200, %rcx
mov $0x0, %rdi
mov 0x0, %rsi
mov %rcx, %rdx
mov $0x200, %rcx
int $0x80
addq $1, 0x16
jmp L3
L3D:
jmpabs $0x200
open_sr:
	.string "# "
