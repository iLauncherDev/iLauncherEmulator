bits 16
section .text
extern main

start:
    mov sp, 0x3000
    mov ax, cs
    mov ds, ax
    xor ax, ax
    mov es, ax
    jmp main