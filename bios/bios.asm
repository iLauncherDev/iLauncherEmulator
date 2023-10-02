section .null
bits 16
section .text
extern main

start:
    mov sp, 0x3000
    mov bx, [0xffff]
    jmp start