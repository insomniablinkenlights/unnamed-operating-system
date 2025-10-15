.code16
.section .text
.globl check_a20;
.globl bios_a20;
.globl kpc_a20;
.globl quick_a20;
check_a20:
pushf
push %ds
push %es
push %di
push %si
cli
xor %ax, %ax
mov %ax, %es
not %ax
mov %ax, %ds
mov $0x500, %di
mov $0x510, %si
movb %es:(%di),%al
push %ax
movb %ds:(%si),%al
push %ax
movb $0x0,%es:(%di)
movb $0xFF,%ds:(%si)
cmpb $0xFF,%es:(%di)
pop %ax
movb %al, %ds:(%si)
pop %ax
movb %al, %es:(%di)
mov $0, %ax
je check_a20__exit
mov $1,%ax
check_a20__exit:
sti
pop %si
pop %di
pop %es
pop %ds
popf
ret

a20wait:
in $0x64,%al
test $2, %al
jnz a20wait
ret

a20wait2:
in $0x64,%al
test $1, %al
jnz a20wait2
ret


kpc_a20:
cli

call a20wait
mov $0xAD, %al
out %al, $0x64

call a20wait
mov $0xd0,%al
out %al,$0x64

call a20wait2
in $0x60, %al
push %eax

call a20wait
mov $0xd1, %al
out %al, $0x64

call a20wait
pop %eax
or $2, %al
out %al, $0x60

call a20wait
mov $0xae, %al
out %al, $0x64

call a20wait
sti
ret

quick_a20:
in $0x92, %al
test $2, %al
jnz quick_a20_after
or $2,%al
and $0xFE,%al
out %al, $0x92
quick_a20_after:
ret
bios_a20:
mov $0x2403,%ax
int $0x15
jb a2015_ns
cmp $0, %ah
jnz a2015_ns
mov $0x2402,%ax
int $0x15
jb a2015_failed
cmp $0,%ah
jnz a2015_failed
cmp $1, %al
jz a2015_activated
mov $0x2401, %ax
int $0x15
jb a2015_failed
cmp $0, %ah
jnz a2015_failed
a2015_failed:
a2015_ns:
a2015_activated:
ret
