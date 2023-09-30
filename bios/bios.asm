bits 16
section .text
extern main

start:
    mov sp, 0x3000
    ;mov ax, cs
    ;mov ds, ax
    ;xor ax, ax
    ;mov es, ax
    call test
    jmp start

test:
    mov word ax, 0xffff
    mov word bx, 0xffff
    push word ax
    mov dword edx, [0xf4f5]
    mov word ax, sp
    mov word sp, 0x0000
    mov word sp, ax
    pop word ax
    int 0x21
    cmp word ax, bx
    je call_loop
    jmp word test

call_loop:
    mov ax, 0xffff
    mov bx, 0xffff
    call loop
    mov ax, 0x0000
    mov bx, 0x0000
    call loop
    jmp call_loop

loop:
    mov word cx, 0x0000
.l1:
    cmp word cx, [0xfff4]
    je .end
    mov dx, 0x0000
.l2:
    int byte 0x20
    add word dx, 1
    cmp word dx, [0xfff2]
    je .l3
    jmp word .l2
.l3:
    add word cx, 1
    jmp word .l1
.end:
    ret