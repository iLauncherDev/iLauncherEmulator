section .null
bits 16
section .text
extern main

start:
    mov sp, 0x3000
    mov dword [esp], 0xffff
    mov dword [esp+ebx], 0xffff
    jmp start