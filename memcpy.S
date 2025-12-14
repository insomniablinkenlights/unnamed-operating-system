.code64
.section .text
.global memcpy;
memcpy:
movq %rdx, %rcx
//shrq $3, %rcx
rep movsb
ret
.global memfill;
memfill:
shrq $3, %rsi
mfl:
movq $0x0, (%rdi)
add $8, %rdi
sub $1, %rsi
cmp $0, %rsi
jne mfl
ret
