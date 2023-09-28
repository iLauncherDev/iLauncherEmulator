section .text

org 0xe0000

start:
    mov ax, cs
    mov ds, ax
    xor ax, ax
    mov es, ax