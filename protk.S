.code32
.section .text
.global _PMMain;
.global idtr;

.macro idte offset, selector, gate, dpl, offset2
.word (\offset+0x7c00)
.word \selector
.byte 0x0
.byte (\gate & 0xf) | (\dpl&0x3)<<5 | 128
.word \offset2
.endm
.equ PML4T_A, 0x10000

.macro gdte limit, base, byte, flags
.word \limit&0xFFFF
.word \base & 0xFFFF
.byte (\base & 0xff0000)>>16
.byte \byte
.byte (\limit&0x70000)>>16|\flags<<4
.byte (\base>>24)&0xFF
.endm 
_PMMain:
//.byte 0x99, 0x99, 0x99, 0x99
//lidt idtr

mov $0x80000001, %eax
cpuid
and $(0x1<<29), %edx
mov $(0x1<<29),%ebx
cmp %edx, %ebx
//fuck it if we don't have x86_64
jne _PMMain

//and now... paging
//right about today is when I should make a backup of my code, LOL
//.equ PML4T_A,  ((0x8c00+_KERNEL_END)&0xfffff000)
mov $PML4T_A, %edi
mov %edi, %cr3
mov $0x10, %eax
mov %eax, %es
xor %eax, %eax
mov $4096, %ecx
//writes 4*sizeofpagetable =4096*4 bytes, for four page tables. this doesn't make much sense to me, because this would still limit it to 4096*512 bytes of paging? says it maps the first 2mb, so i suppose. would you still need to like allocate page tables? that notion frankly scares me
rep stosl
//what?
mov %cr3, %edi
mov $0x10, %eax
mov %eax, %ds
//2mb is one PDT entry (PML4T->PDPT->PDT->PT)
//we can't use the last 3 hex digits, not that we were plannign on it
movl $((PML4T_A+0x1000) |0x3), (%edi)
mov $(PML4T_A+0x1000),%edi

movl $((PML4T_A+0x2000) |0x3), (%edi)
mov $(PML4T_A+0x2000),%edi
movl $((PML4T_A+0x3000) |0x3), (%edi)

//TODO: format of a pt entry?
//512 entries per pt
//8 bytes per entry
//page size 0x1000 = 4kb
mov $(PML4T_A+0x3000), %edi
mov $0x10, %eax
mov %eax, %ds
mov $3, %ebx
mov $511, %ecx
se:
movl %ebx, (%edi)
add $0x1000, %ebx
add $8, %edi
//i don't know if this works
loop se
//page table hopefully all set up, now enable PAE
mov %cr4, %eax
or $(1<<5), %eax
mov %eax, %cr4
//set lm bit
mov $0xc0000080, %ecx
//read model specific register
rdmsr
or $(1<<8), %eax
wrmsr
mov $PML4T_A, %eax
//shl $12, %eax
mov %eax, %cr3
//enable paging
mov %cr0, %eax
or $0x80000001, %eax
mov %eax, %cr0
//compatibility mode is now enabled
lgdt gdtr+0x7c00
mov $0x10, %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs
jmp $0x8, $(_M64+0x7c00)
.code32
//cli
F:
nop
div %bx, %ax
xchgw %bx, %bx
jmp F
idtr:
.word (inth-idtb)-1
.long idtb+0x7c00
//change first two zeroes to size of itdb LATER !
idtb:
//divide error
idte inth, 0x8, 0xF, 0x0, 0x0
//this is honestly STUPID because we assume that our pointer is going to be with offset 0, which makes no sense! it would be a miracle if it worked!
//debug exception
idte inth, 0x8, 0xF, 0x0, 0x0
//nmi
idte inth, 0x8, 0xE, 0x0, 0x0
//breakpoint
idte inth, 0x8, 0xF, 0x0, 0x0
//overflow
idte inth, 0x8, 0xF, 0x0, 0x0
//bound exceeded
idte inth, 0x8, 0xF, 0x0, 0x0
//invalid opcode
idte inth, 0x8, 0xF, 0x0, 0x0
//device not available
idte inth, 0x8, 0xF, 0x0, 0x0
//double fault
idte inth, 0x8, 0xF, 0x0, 0x0
//coprocessor segment overrun
idte inth, 0x8, 0xF, 0x0, 0x0
//invalid tss
idte inth, 0x8, 0xF, 0x0, 0x0
//segment not present
idte inth, 0x8, 0xF, 0x0, 0x0
//stack segment fault
idte inth, 0x8, 0xF, 0x0, 0x0
//gpf
idte inth, 0x8, 0xF, 0x0, 0x0

inth:
xchgw %bx, %bx
iret

//gdtr: .byte 0x0,0x0,0x0,0x0,0x0,0x0
gdtr:
.word tss-gdtb-1
.quad gdtb+0x7c00
gdtb:
gdte 0x0,0x0,0x0,0x0
gdte 0xFFFFF, 0x0, 0x9A, 0xA
gdte 0xFFFFF, 0x0, 0x92, 0xc
gdte 0xFFFFF, 0x0, 0xFA, 0xA
gdte 0xFFFFF, 0x0, 0xF2, 0xC
.word 0x67
.word tss+0x7c00
.byte 0x0
.byte 0x89
.byte 0x0
.word 0x0,0x0, 0x0, 0x0
tss:
.space 0x68, 0x0
