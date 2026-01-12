.code64
.section .text
.global memcpy;
memcpy:
//destination: rdi
//source: rsi
//length: rdx
movq %rdx, %rcx
//shrq $3, %rcx
rep movsb
ret
.global memfill;
memfill:
mfl:
movb $0x0, (%rdi)
add $1, %rdi
sub $1, %rsi
cmp $0, %rsi
jne mfl
ret
