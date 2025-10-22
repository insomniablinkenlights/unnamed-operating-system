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

//set PIC to signal int 0x0-0x7 for 0x20-0x27 as to not map to CPU bs
//call PIC_Remap

//call P_ALLOC
//movq $0x300003, 0x14000
//movq $0x14023, 0x12008
call P_ALLOC2
call FLUSH_TLB
//eax contains a pointer to page info
xchgw %bx, %bx

mov %rax, %rdx
call PCI_SCAN
xchgw %bx, %bx
mov %rax, %rdi
call P_FREE
xchgw %bx, %bx
call P_ALLOC2
mov %rax, %rdi
call constructIDT
//call PIC_Remap
xchgw %bx, %bx
movw $0x20, (idtr_64+0x7c00)
movq %rdi, (idtr_64+0x7c02)
lidt (idtr_64+0x7c00)

//ISA FLOPPY
/*mov $0x70, %dx
mov $(1<<7 | 0x10), %al
xchgw %bx, %bx
outb %al, %dx
inb $0x71, %al*/

//test ISA
//we have a privileged isa pci device so it should emulate it without dma stuff
//outb $0x0, $0x279
//outb $0x0, $0x279
//outb $0x40, $0xA79
/*OUTL:
mov $0x0, %al
mov $0x279, %dx
outb %al, %dx
mov $0x5, %al
outb %al, %dx
mov $0x203, %dx
inb %dx, %al
and $1, %al
cmp $0, %al
je OUTL
mov $0x279, %dx
mov $0x0, %al
outb %al, %dx
mov $0x4, %al
outb %al, %dx
mov $0x203, %dx
inb %dx, %al
*/
	//TODO

L:
xchgw %bx, %bx
jmp L
idtr_64:
.byte 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0

P_ALLOC:
//index of pdpt inside pml4
mov $0, %rax
cmp $512, %rax
je P_ALLOC_ERR
mov $0x200000, %rdx
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
			orq $3, (%r10, %rcx, 8)


					//TODO: fix so that it returns a pointer to the PT struct rather than the struct itself, so to enable P_FREE!!!

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
				andq $0x01, %r9
				cmp $1, %r9
				jne pa_na
				add $1, %ax
				jmp L1
				pa_na:
				//allocate memory with %r8
				movq %rdx, (%r8)
				orq $0x03, (%r8)
				mov %rdx, %r13
				add $0x1000, %rdx
				//prev is a pde
				//allocate memory with (8*%cx+%r10)
				movq %rdx, (%r10, %rcx, 8)
				orq $0x03, (%r10, %rcx, 8)
				//prev is a pdpt
				//prev[0] points to mem alloced in %r8
				mov %r13, %rdx
				//update next p4 entry
				movq %rdx, 8(%r14, %rax, 8)
				orq $0x03, 8(%r14, %rax,8)
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

