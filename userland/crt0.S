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
	pushq %rdi
	pushq %rsi
	call splitc
	mov %rax, (argc)
	popq %rsi
	popq %rdi
	call splitv
	mov (argc), %rdi
	mov %rax, %rsi
	call main
	mov %eax, %edi
	call exit
.size _start, . - _start
argc:
.quad 0x0
