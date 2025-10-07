.code16
.section .text
.globl _start;
.equ ADDROFPAR, 0x7c00+0x1b8+0xc
.equ AMOUNT, _KERNEL_SECTORS+2
_start:
xor %ax,%ax
mov %ax, %ds
mov $0x60, %ax
mov $0x7c00,%si
mov %ax, %es
mov $0, %di
mov $0x1B8,%cx
rep movsb
ljmp $0x60, $aftercopy
aftercopy:
//movb $97, %al
//movb $0x0e, %ah
//int $0x10
/*movb $8, %ah
movb $0, %dl
int $0x13
movb %dh, 0x600
andb $0x3f, %cl
movb %cl, 0x601
*/
mov $0x7c00,%ax
mov %ax,%es
mov $2, %ah
mov $AMOUNT, %al
mov $0, %ch
mov $1,%cl
mov $0,%dh
xor %bx,%bx
mov $0x1, %dl
int $0x13
jnc D

//parse partitions???
jmp R
toCHS:
//cylinder: == LBA mod (HPC * SPT)
movw 0x600, %ax
mov $36,%bl
div %bl
movb %al, 0x602
//head: == (LBA / SPT) % HPC
//div = remainder ah, dividend al
movw 0x600, %ax
mov $18,%bl
div %bl
xor %ah, %ah
mov $2,%bl
div %bl
movb %ah,0x603
//sector: = (lba % spt) +1
movw 0x600,%ax
mov $18,%bl
div %bl
add $1, %ah
mov %ah,0x604
jmp C
R:
movw $1, 0x600
L:
jmp toCHS
C:
/*//
ah=2
al = sector count
ch = cylinder (0x602)
cl = sector (0x604)
dh = head (0x603)
es:bx = buffer
dl = drive number
int 0x13
*///

//movw 0x600,%ax
//shl $5,%ax
//add $0x7C0,%ax
mov $0x600, %ax
mov %ax,%es
xor %bx, %bx
mov $2, %ah
mov $1, %al
xor %cx,%cx
mov 0x602,%ch
mov 0x604, %cl
mov 0x603, %dh
mov $0, %dl
int $0x13

jc D
mov $0x600, %ax
mov %ax, %ds
mov $0x0,%si
mov 0x600,%ax
//add $1, %ax
shl $5,%ax
add $0x7c0,%ax
mov %ax, %es
mov $0x0,%di
mov $0x200,%cx
rep movsb

addw $1, 0x600
movw 0x600, %ax
movw $AMOUNT, %bx
cmpw %ax,%bx
jg L
D:
mov $0x0,%bx
mov %bx, %ds
ljmp $0x7C0,$512

cli
F:
nop
xor %bx, %ax
jmp F
.org 0x1B8
.rept 6
.byte 0 
.endr
.org 0x1BE
.byte 0x80
.byte 0,2,0
.byte 0x7f

.byte 2 
.byte 18
.byte 80
.byte 1,0,0,0
//LBA len:
.quad AMOUNT
.rept 3
.rept 4
.byte 0
.endr
.endr
.org 0x1FE
.byte 0x55,0xAA
