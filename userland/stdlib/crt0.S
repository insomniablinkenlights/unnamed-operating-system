.section .text
.global _start
_start:
	//set up end of stack frame linked list
	mov $0, %rbp
	pushq %rbp
	pushq %rbp
	movq %rsp, %rbp


	pushq %rsi
	pushq %rdi
	call initialize_standard_library
	call _init
	popq %rdi
	popq %rsi
	call main
	mov %rax, %rdi
	call exit
.size _start, . - _start
