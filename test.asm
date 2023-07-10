bits 16
section .bss

stack_start:
    resb 0x1000
stack_end:

section .text

start:
    mov sp, stack_end
    mov ax, 0x00
    mov bx, 0x00
    mov cx, 0x00
    mov dx, 0x00

loop:
    int 0x20
    add ax, 0x02
    add bx, 0x04
    add cx, 0x08
    add dx, 0x01
    jmp loop

bits 32

PMODE:
    jmp PMODE