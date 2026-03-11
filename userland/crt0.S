.code64
.text
.globl _start
_start:
	movq $0, %rbp
	pushq %rbp
	pushq %rbp
	movq %rsp, %rsp
	pushq %rsi
	pushq %rdi
	call initialize_standard_library
//	call _init
	//restore argc and argv
	popq %rdi
	popq %rsi
	call main
	mov %eax, %edi
	call exit
.size _start, . - _start
