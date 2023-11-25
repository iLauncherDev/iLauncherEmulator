bits 16
section .text
extern main

start:
    mov sp, 0x3000
    mov ax, cs
    mov ds, ax
    xor ax, ax
    mov es, ax
    call main
.loop:
    jmp .loop

bits 16
section .null
reset:
    cli
    cld
    jmp 0xe000:0x00

times 0x0f - ($ - $$) db 0x00