.code64
.section .text
.global outb;
.global inb;
.global FLUSH_TLB;
.global FAULT;
.global TIMER_INTERRUPT;
.global IDT_DF;
.global switch_to_task;
.global current_task_TCB;
.global CLI;
.global STI;
.global HLT;
.macro UNCLOBBER
push %rax
push %rcx
push %rdx
push %r8
push %r9
push %r10
push %r11
push %rsi
push %rdi
.endm
.macro RECLOBBER
pop %rdi
pop %rsi
pop %r11
pop %r10
pop %r9
pop %r8
pop %rdx
pop %rcx
pop %rax
.endm
outb:
push %rdx
push %rax
mov %di, %dx
mov %si, %ax
outb %al, %dx
pop %rax
pop %rdx
ret
inb:
push %rdx
xor %rax, %rax
mov %di, %dx
inb %dx, %al
pop %rdx
ret
FLUSH_TLB:
push %rax
mov %cr3, %rax
mov %rax, %cr3
pop %rax
ret
FAULT:
xchgw %bx, %bx
nop
jmp FAULT

IDT_ERR_H:
xchgw %bx, %bx
mov $1, %bx
jmp FAULT
nop
iretq
TIMER_INTERRUPT:
UNCLOBBER
push %rdi
xor %rdi, %rdi
call PIC_sendEOI
pop %rdi
call PIT_IRQ_handler
RECLOBBER
iretq

IDT_DF:
xchgw %bx, %bx
pop %rbx
mov $0x8, %bx
jmp FAULT
iretq

switch_to_task:
push %rbx
push %rsi
push %rdi
push %rbp
//might also need to save r9-15

movabsq $(current_task_TCB), %rax
mov (%rax), %rbx
cmp $0x0, %rbx
je ST2

mov %rsp, (%rbx)
movb 24(%rbx), %cl
cmp $0x0, %cl
jne ST2
//replace on list if it's still running
movb $1, 24(%rbx)
movabsq $(LAST_THREAD), %rcx
mov (%rcx), %rdx
mov %rbx, 16(%rdx)
mov %rbx, (%rcx)
jmp ST3
ST2:
//i don't know why this is important...
ST3:
//load next state
mov %rdi, %rsi
movb $0, 24(%rsi)
mov %rsi, (%rax)
mov (%rsi), %rsp
//TODO: do some TSS stuff
//TODO: do some cr3 stuff

pop %rbp
pop %rdi
pop %rsi
pop %rbx
ret

CLI:
cli
ret

STI:
sti
ret

HLT:
hlt
ret

.global ERROR
ERROR:
xchgw %bx, %bx
push %rdi
push %rsi
call FAULT

.global jump_usermode
jump_usermode:
mov $(3|(4*8)), %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs

mov %rsp, %rax
push $(3|(4*8))
push %rax
pushf
push $(3|(3*8))
push %rdi
iretq


