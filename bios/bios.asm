section .null
bits 16
section .text
extern main

start:
    mov sp, 0x3000
    mov ecx, 0xdeadc0de
    call main
.loop:
    jmp .loop