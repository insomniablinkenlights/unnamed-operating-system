.code64
.section .text
.global PCI_CONFIG_READ_WORD
.global PCI_CHECK_DEVICE
.global PCI_SCAN
PCI_CONFIG_READ_WORD:
	//al: bus, ah: slot, bl: func, bh: offset
	//returns word of god in eax
	push %rbx
	push %rcx
	push %rdx

	movb %bh, (_PCRW_O+0x7c00)
	movl $0x80000000, (_PCRW_AD+0x7c00)
	andb $2, (_PCRW_O+0x7c00)
	shlb $3, (_PCRW_O+0x7c00)	
	and $0xFCFF, %ebx
	xchgb %bl, %bh
	orl %ebx, (_PCRW_AD+0x7c00)
	mov %ah, %bl
	and $0xFF, %ebx
	shl $11, %ebx
	orl %ebx, (_PCRW_AD+0x7c00)
	and $0xFF, %eax
	shl $16, %eax
	orl %eax, (_PCRW_AD+0x7c00)
	movl (_PCRW_AD+0x7c00), %eax
	mov $0xcf8, %dx
	outl %eax, %dx
	mov $0xCFC, %dx
	inl %dx, %eax
	movb (_PCRW_O+0x7c00), %cl
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
	//returns vendor in eax and device in r12
	//al: bus, ah: slot, bl: function
	push %rbx
	push %rcx
	push %rax
	xor %bh, %bh
//	xchgw %bx, %bx
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
	pop %rax
	cmp $0xFFFF, %ecx
	je _PCV_R
	// if vendor isn't nonexistent, read the next word to clear
	//xor %bh, %bh
	mov $2, %bh
	call PCI_CONFIG_READ_WORD
	mov %rax, %r12
	_PCV_R:
	mov %ecx, %eax
	pop %rcx
	pop %rbx
	ret
PCI_CHECK_FUNCTION:
	//al: bus, ah: device, bl: function
	//returns class code, subclass, prog if, in %ecx
	push %rax
	push %rbx
	push %rax
	mov $0x8, %bh
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
	pop %rax
	mov $0xA, %bh
	call PCI_CONFIG_READ_WORD
	shl $16, %eax
	or %eax, %ecx
	and $0xFFFFFF00, %ecx
	shr $8, %ecx
	and $0x00FFFFFF, %ecx
	pop %rbx
	pop %rax
	ret
	
PCI_GET_HEADER_TYPE:
	//al: bus, ah: device, bl: function
	//returns header type in %ecx
	push %rax
	push %rbx
	mov $0xE, %bh
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
	and $0xFF, %ecx
	pop %rbx
	pop %rax
	ret
PCI_CHECK_DEVICE:
	//al: bus, ah: device, edx:memory location to put stuff in
	//returns device functions in %ds:(%rdx), taking a 32-byte area
	//null device functions will have top = 0xFF, otherwise top = 0x00
	push %rdx
	push %r8
	push %rbx
	push %rax
	push %rax
	xor %rbx, %rbx
	_PCD_F_L:
	movl $0xFFFFFFFF, (%rdx, %rbx, 4)
	add $1, %ebx
	cmp $8, %ebx
	jne _PCD_F_L
	xor %ebx, %ebx
	xor %r8, %r8
	//xchgw %bx, %bx
	movb $0, (_PCD_FUNCTION+0x7c00)
	xor %bl, %bl
	call PCI_CHECK_VENDOR
	cmp $0xFFFF, %eax
	je _PCD_R
	pop %rax
	push %rax
	call PCI_CHECK_FUNCTION
	movl %ecx, (%rdx, %r8, 4)
	add $1, %r8
	call PCI_GET_HEADER_TYPE
	and $0x80, %ecx
	cmp $0x0,%ecx
	je _PCD_R
	_PCD_FN_L:
	addb $1, (_PCD_FUNCTION+0x7c00)
	movb (_PCD_FUNCTION+0x7c00), %bl
	//xchgw %bx, %bx
	call PCI_CHECK_VENDOR
	cmp $0xFFFF, %eax
	pop %rax
	push %rax
	je _PCD_R2
	call PCI_CHECK_FUNCTION
	movl %ecx, (%rdx, %r8, 4)
	_PCD_R2:
	add $1, %r8
	cmp $7, %bl
	jne _PCD_FN_L
	
	_PCD_R:
	pop %rax
	pop %rax
	pop %rbx
	pop %r8
	pop %rdx
	
	ret
	_PCD_FUNCTION:
	.byte 0x0
PCI_SCAN:
	push %rax
	push %rdx
	push %rbx
	push %rcx
	push %r8
	xor %rax, %rax
	xor %rbx, %rbx
	xor %r8, %r8
	xor %r11, %r11
	//r8: bus index, r9: slot index, r10: func index
	_PS_B:
		xor %r9, %r9
		_PS_S:
			xor %r10, %r10
			mov %r8b, %al
			mov %r9b, %cl
			mov %cl, %ah
			xor %bl, %bl
			call PCI_CHECK_VENDOR
			cmp $0xFFFF, %eax
			je _PS_S_FI
			

	
			mov %r8b, %al
			mov %r9b, %cl
			mov %cl, %ah
			xor %bl, %bl
			call PCI_D_STATS
			call PCI_GET_HEADER_TYPE
			and $0x80, %ecx
			cmp $0x0, %ecx
			je _PS_S_FI
			_PS_F:
			add $1, %r10
			mov %r10b, %bl
			mov %r8b, %al
			mov %r9b, %cl
			mov %cl, %ah
			call PCI_CHECK_VENDOR
			cmp $0xFFFF, %eax
			je _PS_F_E
			mov %r8b, %al
			mov %r9b, %cl
			mov %cl, %ah
			call PCI_D_STATS
			_PS_F_E:
			cmp $7, %r10
			jne _PS_F
			_PS_S_FI:
			add $1, %r9
			cmp $31, %r9
			je _PS_S_FI2
			jmp _PS_S
			_PS_S_FI2:
			add $1, %r8
			cmp $255, %r8
			je _PS_FI
			jmp _PS_B

	_PS_FI:	
	movl $0xFFFFFFFF, (%rdx, %r11,4)
	pop %r8
	pop %rcx
	pop %rbx
	pop %rdx
	pop %rax
	ret
PCI_D_STATS:
	//al bus, ah device, bl function, rdx+r11*4 location
	push %r12
	push %rax
	push %rbx
	push %rcx
	push %rax
	push %rbx
	and $0xFFFF, %rax
	xchg %al, %ah
	shl $8, %eax 
	mov %bl, %al
	movl %eax, (%rdx,%r11,4)
	pop %rbx
	pop %rax
	push %rax
	call PCI_CHECK_VENDOR
	shl $16, %eax
	or %r12, %rax
	movl %eax, 4(%rdx, %r11,4)
	pop %rax
	call PCI_CHECK_FUNCTION
	movl %ecx, 8(%rdx, %r11,4)
	call PCI_GET_HEADER_TYPE
	movl %ecx, 12(%rdx, %r11, 4)
	add $4, %r11
	pop %rcx
	pop %rbx
	pop %rax
	pop %r12
	ret
