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
.global BREAK;
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
.global FLUSH_TLB2
FLUSH_TLB2:
push %rax
push %rbx
movabsq $(current_task_TCB), %rax
movq (%rax), %rax
movq 60(%rax), %rax
movabsq $0x8000030000, %rbx
movq 8(%rbx), %rbx
mov %rbx, 8(%rax)
mov %cr3, %rax
mov %rax, %cr3
pop %rbx
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
//rbx is a pointer to the current task
cmp $0x0, %rbx
//if we're null then we skip
je ST2

//load rsp into old task
mov %rsp, (%rbx)
//load state from old task into cl
movb 24(%rbx), %cl
//if state is other than 0 (running) then we don't need to replace it onto the list
cmp $0x0, %cl
jne ST2

//replace on list if it's still running
//make it ready
movb $1, 24(%rbx)
movabsq $(LAST_THREAD), %rcx
movq (%rcx), %rdx
//last_thread->next = old
movq %rbx, 16(%rdx)
//last_thread = old
movq %rbx, (%rcx)
//old->next = NULL
movq $0, 16(%rbx)
jmp ST3
ST2:
//i don't know why this is important...
ST3:
//load next state
//rdi = *next
mov %rdi, %rsi
//set rsi to running
movb $0, 24(%rsi)
//rax = **tcb
//replace current task tcb
movq (%rax), %rbx
//rbx = *tcb
//replace current_task_TCB
movq %rsi, (%rax)
//replace RSP with next's rsp
movq (%rsi), %rsp
//get cr3
movq 60(%rsi), %rcx
movq 60(%rbx), %rax
cmp %rax, %rcx
je STDONE
//they have diff' cr3s
//swap user mem in...
//rcx is *new, rax is *old
//rbx is *first
movabsq $(0x8000030000), %rbx
movq (%rbx), %rsi
movq %rsi, (%rax)
//new to first
movq (%rcx), %rsi
movq %rsi, (%rbx)
STDONE:
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

.global DIV64_32
DIV64_32:
mov %rdi, %rax
shr $32, %rdi
mov %rdi, %rdx
div %esi
ret
.global MOD64_32
MOD64_32:
mov %rdi, %rax
shr $32, %rdi
mov %rdi, %rdx
div %esi
xor %rax, %rax
mov %edx, %eax
ret
BREAK:
xchgw %bx, %bx
xchgw %cx, %cx
push %rdi
pop %rdi
ret
.global LTR
LTR:
xchgw %bx, %bx
mov %rdi, %rax
ltr %ax
ret
.macro CLEARALLREGS
xor %rax, %rax
xor %rbx, %rbx
xor %rcx, %rcx
xor %rdx, %rdx
xor %rsi, %rsi
xor %rdi, %rdi
.endm
.global ASMS_UM
ASMS_UM:
mov %rsi, %r9
mov $(0x20 |3), %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs
mov %rsp, %rax
push $(0x20|3)
push $0x30000
pushf
push $(0x18|3)
push %rdi
CLEARALLREGS
mov %r9, %rsi
iretq
.global get_rspplus1
get_rspplus1:
mov %rsp, %rax
ret
.global Int0x80Handler
Int0x80Handler:
//arguments are passed in standard sysv style
//already on stack: ss, sp, flags, cs, ip
//need to push ax, gs, fs, es, ds, -ENOSYS, bp, di, si, dx, cx, bx
//the problem is that if we get an interrupt while our TSS's rsp0 is set to this we get a lot of funny shit, I think, so I should find what rsp0 is before this shit
//yeah, I'm almost completely sure that the issue is that getting an interrupt while in our int 0x80 handler is loading into the stack OF the handler, corrupting shit -- to fix this we'll need to have one initial stack which loads into our actual stack
//I think the best thing to do is maybe make a new stack for the next interrupt?
//just load in a new rsp0 to the tss when we're using the one it currently has, we then basically take that rsp0 for ourselves ^w^
cli
push %rcx
push %rdx
push %r8
push %r9
push %r10
push %r11
push %rsi
push %rdi
call INT0x80C
pop %rdi
pop %rsi
pop %r11
pop %r10
pop %r9
pop %r8
pop %rdx
pop %rcx
sti
iretq
.global PS2_HANDLER
PS2_HANDLER:
UNCLOBBER
mov $1, %rdi
call unblockirq
RECLOBBER
iretq
.global PS2_HANDLER12
PS2_HANDLER12:
UNCLOBBER
mov $12, %rdi
call unblockirq
RECLOBBER
iretq
