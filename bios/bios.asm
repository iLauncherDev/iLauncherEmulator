section .null
bits 16
section .text
extern main

start:
    mov sp, 0x3000
    call main
    jmp start