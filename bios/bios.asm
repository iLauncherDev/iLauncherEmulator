section .null
bits 16
section .text
extern main

start:
    mov esp, 0x3000
    call main
.loop:
    jmp .loop