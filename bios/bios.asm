section .null
bits 16
section .text
extern main

start:
    mov eax, cr0
    mov eax, 1
    mov cr0, eax
bits 32
    call main
.loop:
    jmp .loop