bits 16
section .bss

stack_start:
    resb 0x1000
stack_end:

section .text

start:
    mov word sp, stack_end
    push word ax
    mov word ax, sp
    mov word sp, 0x00
    mov word sp, ax
    pop word ax
    mov word bp, 0x00
    mov word bx, 0x00
    mov word cx, 0x00
    mov word dx, 0x00

loop:
    int byte 0x20
    add word ax, 0x02
    add word bx, 0x04
    add word cx, 0x08
    add word dx, 0x01
    jmp word loop

bits 32

PMODE:
    jmp PMODE