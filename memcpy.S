.code64
.section .text
.global memcpy;
memcpy:
movq %rdx, %rcx
shrq $3, %rcx
rep movsq
ret
