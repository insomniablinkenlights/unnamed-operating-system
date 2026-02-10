.code64
.section .text
L:
mov $open_sr, %rdx
movb $0x11, 2(%rdx)
mov $1, %rdi
mov $0, %rsi
mov $3, %rcx
int $0x80

movq $0, 0x0
L2:
mov $0, %rdi
mov $0, %rsi
movq 0x0, %rdx
add $0x200, %rdx
mov $0x1, %rcx
int $0x80

mov $1, %rdi
mov $0, %rsi
movq 0x0, %rdx
add $0x200, %rdx
mov $0x1, %rcx
int $0x80
addq $1, 0x0
jmp L2
//movq $0, 0x0
/*L2:
//READ(0, *(0x0)+0x200, 1)
mov $0, %rsi
mov $0, %rdi
mov 0x0, %rdx
add $0x200, %rdx
mov $4, %rcx
xchgw %bx, %bx
int $0x80
//read 4 bytes from stdin to 0x200
//if it's pressed, we don't press it
//if( *(*(0x0)+0x202) == 1) goto l2
mov 0x0, %rdx
add $0x202, %rdx
movb (%rdx), %dl
cmp $0x1, %dl
je L2
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
mov $0x7, %rdi
mov $0x200, %rsi
sub $0x200, %rdx
int $0x80
//exec it, rax is now the fd of our stdio
mov $0x8, %rdi
mov %rax, 0x0
mov %rax, %rsi
mov $0x0, %rdx
int $0x80
//bound one way
mov 0x0, %rdx
mov $0x0, %rsi
mov $0x8, %rdi
int $0x80 
//exit!
mov $0x9, %rdi
int $0x80*/
open_sr:
	.string "# "
