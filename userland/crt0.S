.code64
.text
.globl _start
_start:
	xchgw %bx, %bx
	xor %ebp, %ebp
	mov (%rsp), %edi
	lea 8(%rsp), %rsi
	lea 16(%rsp, %rdi, 8), %rdx
	xor %eax, %eax
	call main
	mov %eax, %edi
	xor %eax, %eax
	mov $0x9, %rdi
	int $0x80
movabsq $0x80000000, %rax
movq $0, (%rax)
