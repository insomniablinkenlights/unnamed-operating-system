.code64
.section .text
.macro CIDTE name, no
.global \name
\name:
xchgw %bx, %bx
mov $\no, %bx
call FAULT
iretq
.endm
.macro CIDTER name, no
.global \name
\name:
xchgw %bx, %bx
mov $\no, %bx
pop %rbx
call FAULT
iretq
.endm

CIDTE IDT_DE, 0x0
CIDTE IDT_DB, 0x1
CIDTE IDT_NMI, 0x2
CIDTE IDT_BP, 0x3
CIDTE IDT_OF, 0x4
CIDTE IDT_BR, 0x5
CIDTE IDT_UD, 0x6
CIDTE IDT_NM, 0x7
//IDT_DF implemented in asmbridge.S
//0x09 is reserved
CIDTER IDT_TS, 0xA
CIDTER IDT_NP, 0xb
CIDTER IDT_SS, 0xc
CIDTER IDT_GP, 0xd
//IDT_PF implemented later in file
//0x0f is reserved
CIDTE IDT_MF, 0x10
CIDTER IDT_AC, 0x11
CIDTE IDT_MC, 0x12
CIDTE IDT_XM, 0x13
CIDTE IDT_VE, 0x14
CIDTER IDT_CP, 0x15
//timer interrupt implemented in asmbridge.S
//floppy interrupt implemented later in file
//both ps/2 interrupts implemented in asmbridge.S

.global IDT_PF
IDT_PF:
//xchgw %bx, %bx
//mov $0xe, %bx
//pop %rbx
//mov %cr2, %rbx
//pop %rbx
//call FAULT
//pushes an error code which we need to somehow get rid of before the return
//stack: error code
push %rsi
mov 8(%rsp), %rsi
push %rax
push %rcx
push %rdx
push %rdi
push %r8
push %r9
push %r10
push %r11
mov %cr2, %rdi
call PAGE_FAULT
pop %r11
pop %r10
pop %r9
pop %r8
pop %rdi
pop %rdx
pop %rcx
pop %rax
pop %rsi
add $8, %rsp
iretq

.global FL
FL:
push %rax
push %rcx
push %rdx
push %rsi
push %rdi
push %r8
push %r9
push %r10
push %r11
mov $6, %rdi
call unblockirq
pop %r11
pop %r10
pop %r9
pop %r8
pop %rdi
pop %rsi
pop %rdx
pop %rcx
pop %rax
iretq
.global SPIRQ
SPIRQ:
xchgw %bx, %bx
mov $0x27, %rbx
iretq
