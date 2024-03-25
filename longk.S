.code64
.section .text
.global _M64;
_M64:
L:
xchgw %bx, %bx
jmp L
