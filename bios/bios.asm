section .null
bits 16
section .text
extern main

start:
    mov esp, 0x3000
    call dword main
.loop:
    hlt
    jmp dword .loop