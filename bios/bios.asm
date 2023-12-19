bits 16
section .text
extern main

start:
    inc eax
    jmp start
    mov sp, 0x3000
    mov ax, cs
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov eax, cr0
    or eax, 1
    mov cr0, eax
bits 32
    call main
.loop:
    jmp .loop

bits 16
section .null
reset:
    jmp 0xe000:0x00

times 0x0f - ($ - $$) db 0x00
