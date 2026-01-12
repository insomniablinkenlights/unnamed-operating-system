.code64
.section .text
.global _M64;
_M64:
mov $0x8000007bff, %rsp
mov $0x8000007bff, %rbp
/*
mov $0x80000008, %eax
//cpuid
and $0xFF00, %eax
shr $8, %eax*/
mov $0x10, %bx
mov %bx, %ss


//SCAN PCI DEVICES
call P_ALLOC3_SETUP
call KPALLOC
mov %rax, %rdx
call PCI_SCAN

//CONSTRUCT IDT
call KPALLOC
mov %rax, %rdi
push %rax
call constructIDT
pop %rax
cli
movabsq $idtr_64, %r9
movq $(0x27*16), (%r9)
movq %rax, 2(%r9)
lidt (%r9)
//set PIC to signal int 0x0-0x7 for 0x20-0x27 as to not map to CPU bs
call PIC_Remap
call PIT_SETFRQ
cli

cli
call initialize_multitasking
//only one kernel task currently

sti
//test floppies -- remove this later probably
//movabsq $0, %rdi
//mov $0x1000, %rsi
//call floppy_read
//mov $0, %rdi
//mov $0x1000, %rsi
//mov %rax, %rdx
//call floppy_write
call memory_operation_test
call floppy_driver_test
//start init task!
call start_init_task
//we PF here because L is in a kernel segment -- security !!!
L:
nop
nop
xchgw %bx, %bx
jmp L
//rax: L2
/*L:
//push %rax
//mov %rax, %rdi
//call switch_to_task
nop
mov $1000000000, %rdi
call nano_sleep
xchgw %bx, %bx
//pop %rax
jmp L
L2:
xchgw %bx, %bx
mov $1000000000, %rdi
call nano_sleep
//push %rax
//movabsq $(0x8000000000+0x1d000), %rdi
//call switch_to_task
//pop %rax
jmp L2*/
idtr_64:
.byte 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0

