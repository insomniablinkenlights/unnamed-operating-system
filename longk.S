.code64
.section .text
.global _M64;
_M64:
/*
paging setup
four level tree, first node 0x10000
we need a system to check if a node is taken
page format?
*/
mov $0x80000008, %eax
cpuid
and $0xFF00, %eax
shr $8, %eax
xchgw %bx, %bx

call P_ALLOC
xchgw %bx, %bx
call P_ALLOC
xchgw %bx, %bx
L:
xchgw %bx, %bx
jmp L


P_ALLOC:
//index of pdpt inside pml4
mov $0, %rax
cmp $512, %rax
je P_ALLOC_ERR
mov $0x14000, %rdx
L1:
	mov $0, %rbx
	//index of pde inside pdpt inside pml4
	L2:
		mov $0, %rcx
		//index of pt inside pde inside pdpt inside pml4
		L3:
			add $0x1000, %rdx
			//pml4
			movq $0x10000, %r14
			movq (%r14,%rax,8), %r8
			andq $0x3FFFF000, %r8
			//pdpt
			movq (%r8,%rbx,8), %r8
			andq $0x3FFFF000, %r8
			mov %r8, %r10	
			//pde
			movq (%r8, %rcx,8), %r8
			//andq $0x3FFFF000, %r8
			//pt
			cmp $0, %r8
			je NP
			jmp P_A_C_4
			movq (%r8), %r9
			and $1, %r9
			cmp $1, %r9
			je P_A_C_4
			NP:
			//not present:
			movq %rdx, (%r10, %rcx, 8)	
			orq $1, (%r10, %rcx, 8)
			movq %r10, %rax
			shl $3, %rcx
			add %rcx, %rax
			jmp P_ALLOC_END
			P_A_C_4:
			add $1, %cx
			cmp $510, %cx
			jne L3
			add $1, %bx
			//511
			//is pdpt full?
			cmp $511, %bx
			jne L2
			cnpdpt:
				movq 8(%r14, %rax, 8), %r9
				orq $0x01, %r9
				cmp $1, %r9
				jne pa_na
				add $1, %ax
				jmp L1
				pa_na:
				//allocate memory with %r8
				movq %rdx, (%r8)
				orq $0x01, (%r8)
				mov %rdx, %r13
				add $0x1000, %rdx
				//prev is a pde
				//allocate memory with (8*%cx+%r10)
				movq %rdx, (%r10, %rcx, 8)
				orq $0x01, (%r10, %rcx, 8)
				//prev is a pdpt
				//prev[0] points to mem alloced in %r8
				mov %r13, %rdx
				//update next p4 entry
				movq %rdx, 8(%r14, %rax, 8)
				orq $0x01, 8(%r14, %rax,8)
				add $0x1000, %rdx
				add $0x1, %ax
				jmp L1		
				
				//is next pdpt allocated?
					//we're good, add 1 to ax and jmp l1
					//allocate it, return first of first
				//just add 1 to bx and jmp l2
							
				
				 

P_ALLOC_END:
ret
P_ALLOC_ERR:
xchgw %bx, %bx

ret
address_width:
.byte 0x0
