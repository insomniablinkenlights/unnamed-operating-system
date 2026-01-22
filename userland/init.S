.code64
.section .text
L:
xchgw %bx, %bx
mov $filename_str, %rsi
mov $0x0, %rdx
//flags not yet supported...
mov $0x2, %rdi
int $0x80
//open /sbin/txt
xchgw %bx, %bx
mov %rax, %rsi
mov %rax, 0x0
mov $0x200, %rdx
mov $0x400, %rcx
mov $0x0, %rdi
int $0x80
//read 0x400 bytes to 0x200
xchgw %bx, %bx
mov 0x0, %rsi
mov $0x3, %rdi
int $0x80
//close /sbin/txt
xchgw %bx, %bx
mov $0x200, %rdx
mov $1, %rdi
mov $0, %rsi
mov $0x400, %rcx
int $0x80
//write 0x400 bytes from 0x200 to stdout
xchgw %bx, %bx
jmp L
filename_str:
	.string "/sbin/txt"
