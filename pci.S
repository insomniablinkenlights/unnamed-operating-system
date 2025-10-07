.code64
.section .text
.global PCI_CONFIG_READ_WORD
.global PCI_CHECK_DEVICE
PCI_CONFIG_READ_WORD:
	//al: bus, ah: slot, bl: func, bh: offset
	//returns word of god in eax
	push %rbx
	push %rcx
	push %rdx

	movb %bh, (_PCRW_O+0x7c00)
	andb $2, (_PCRW_O+0x7c00)
	shlb $3, (_PCRW_O+0x7c00)	
	and $0xFFFC, %ebx
	orl %ebx, (_PCRW_AD+0x7c00)
	mov %ah, %bl
	shl $11, %ebx
	orl %ebx, (_PCRW_AD+0x7c00)
	and $0xFF00, %eax
	shl $16, %eax
	or %eax, (_PCRW_AD+0x7c00)
	movl (_PCRW_AD+0x7c00), %eax
	mov $0xcf8, %dx
	out %eax, %dx
	mov $0xCFC, %dx
	inl %dx, %eax
	mov (_PCRW_O+0x7c00), %cl
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
	//al: bus, ah: slot, bl: func
	push %rbx
	push %rcx
	push %rax
	xor %bh, %bh
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
	pop %rax
	cmp $0xFFFF, %ecx
	je _PCV_R
	// if vendor isn't nonexistent, read the next word to clear
	xor %bh, %bh
	mov $2, %bh
	call PCI_CONFIG_READ_WORD
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
	mov $0x8, %bh
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
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
	mov $0xC, %bh
	call PCI_CONFIG_READ_WORD
	mov %eax, %ecx
	and $0xFF0000, %ecx
	shr $16, %ecx
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
	mov (_PCD_FUNCTION+0x7c00), %bl
	call PCI_CHECK_VENDOR
	cmp $0xFFFF, %eax
	je _PCD_R
	pop %rax
	push %rax
	call PCI_CHECK_FUNCTION
	mov %ecx, (%rdx, %r8, 4)
	add $1, %r8
	call PCI_GET_HEADER_TYPE
	and $0x80, %ecx
	cmp $0x0,%ecx
	je _PCD_R
	_PCD_FN_L:
	addb $1, (_PCD_FUNCTION+0x7c00)
	mov (_PCD_FUNCTION+0x7c00), %bl
	call PCI_CHECK_VENDOR
	cmp $0xFFFF, %eax
	je _PCD_R
	pop %rax
	push %rax
	call PCI_CHECK_FUNCTION
	mov %ecx, (%rdx, %r8, 4)
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
