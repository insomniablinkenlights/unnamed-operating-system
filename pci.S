.code64
.section .text
.global PCI_CONFIG_READ_WORD
PCI_CONFIG_READ_WORD:
	//al: bus, ah: slot, bl: func, bh: offset
	//returns word of god in eax
	push %rbx
	push %rcx
	push %rdx

	movb %bh, (_PCRW_O)
	andb $2, (_PCRW_O)
	shlb $3, (_PCRW_O)	
	and $0xFFFC, %ebx
	orl %ebx, (_PCRW_AD)
	mov %ah, %bl
	shl $11, %ebx
	orl %ebx, (_PCRW_AD)
	and $0xFF00, %eax
	shl $16, %eax
	or %eax, (_PCRW_AD)
	movl (_PCRW_AD), %eax
	mov $0xcf8, %dx
	out %eax, %dx
	mov $0xCFC, %dx
	inl %dx, %eax
	mov (_PCRW_O), %cl
	shr %cl, %eax
	and $0xFFFF, %eax

	pop %rdx
	pop %rcx
	pop %rbx
	ret
	_PCRW_AD:
	.long 0x80000000
	_PCRW_O:
	.byte 0x0
PCI_CHECK_VENDOR:
	//returns vendor in eax
	//al: bus, ah: slot
	push %rbx
	push %rcx
	push %rax
	xor %ebx, %ebx
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
	pop %rax
	cmp $0xFFFF, %ecx
	je _PCV_R
	// if vendor isn't nonexistent, read the next word to clear
	xor %ebx, %ebx
	mov $2, %bh
	call PCI_CONFIG_READ_WORD
	_PCV_R:
	pop %rbx
	mov %ecx, %eax
	pop %rcx
	ret
	
