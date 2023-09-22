bits 16
org 0xf0000

section .text

pmode:use16
    mov word sp, 0x3000
    call word start
    lgdt [gdtr]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
	jmp	0x08:.flush
.flush:use32
    mov dword eax, eax
	jmp dword .flush

start:use16
    mov word ax, 0xffff
    mov word bx, 0xffff
    push word ax
    mov word dx, [0xf4f5]
    mov word ax, sp
    mov word sp, 0x0000
    mov word sp, ax
    pop word ax
    int 0x21
    cmp word ax, bx
    je call_loop
    jmp word start

call_loop:use16
    mov word ax, 0xffff
    mov word bx, 0xffff
    call loop
    mov word ax, 0x0000
    mov word bx, 0x0000
    call loop
    jmp call_loop

loop:use16
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

gdt:
    .start:
        dq 0x0000000000000000
        dq 0x00cf9a000000ffff
        dq 0x00cf93000000ffff
    .end:
    
gdtr:
    dw gdt.end - gdt.start
    dd gdt.start