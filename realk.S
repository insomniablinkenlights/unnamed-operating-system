.code16
.section .text
.global _kernelmain;

.macro gdte limit, base, byte, flags
.word \limit&0xFFFF
//0,1
.word \base & 0xFFFF
//2,3
.byte (\base & 0xFF0000)>>16
//4
.byte \byte
//5
.byte (\limit&0x70000)>>16 | \flags<<4
//6
.byte (\base >>24)&0xFF
//7
//.word 0x00
.endm

_kernelmain:

//we are at 0x7e00

//first, we set up the stack, at 0x7BFF (?) which is about 29.75 kb and growing downwards to 0x500.
mov $0x7BFF, %sp
//notify bios that we will be in 64 bit
mov $0xec00, %ax
mov $2, %bl
//int $0x15
mov $0x7bff, %sp
/*struct data area
low memory (kb, starting from 0) | 2 bytes
*/
call check_a20
cmp $1,%ax
je A20Enabled
call bios_a20
call check_a20
cmp $1,%ax
je A20Enabled
call kpc_a20
call check_a20
cmp $1,%ax
je A20Enabled
call quick_a20
call check_a20
cmp $1,%ax
je A20Enabled
in $0xee,%al
call check_a20
cmp $1,%ax
je A20Enabled

//error handling?

A20Enabled:

cli
//we SHOULD disable NMI here, but I'm lazy.
//setGDT:
xor %eax, %eax
mov %ds, %ax
shl $4, %eax
add $(gdtb+0x00), %eax
mov %eax, (gdtr+0x02)
mov $0x30, %eax
mov %ax, (gdtr+0x00)
lgdt (gdtr+0x00)
lidt idtr+0x00
mov %cr0, %eax
or $1, %al
mov %eax, %cr0
//.code32 ljmp *(_PMMain+0x00)
//ljmp *_PMMain
//xchgw %bx,%bx
//.code32 jmpl $0x8, $(_PMMain+0x00)
.byte 0xea
.word _PMMain+0x00
.byte 0x08, 0x00

xor %eax, %eax

cli
F:
nop
xchgw %bx, %bx
xor %bx, %bx
jmp F
gdtr:
.byte 0x0,0x0,0x0,0x0,0x0,0x0
gdtb:
gdte 0x0,0x0,0x0,0x0
gdte 0xFFFFF,0x0,0x9A,0xC
gdte 0xFFFFF,0x0,0x92,0xC
gdte 0xFFFFF,0x0,0xfA,0xC
gdte 0xFFFFF,0x0,0xf2,0xC
//add TSS
//.word (tss+0x7e00)
//.word 0x6B
//.byte 0
//.byte 0x89
//.byte 0x0
//.word 0x0
.byte 0x6B
.byte 0x0
.word tss+0x00
.byte 0x0
.byte 0x89
.byte 0x0
.byte 0x0
//gdte (tss+0x7c00), 0x6B, 0x89, 0x0
tss:
.space 0x6C, 0x0

