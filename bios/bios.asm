section .null
bits 16
section .text
extern main

start:
    mov esp, 0x3000
.loop:
    call word test
    jmp dword .loop

test:
    ret