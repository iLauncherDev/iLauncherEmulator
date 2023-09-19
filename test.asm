bits 16
section .bss

stack_start:
    resb 0x1000
stack_end:

section .text

pmode:use16
    mov word sp, stack_end
    call start
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
    mov word dx, 0x0000
.l1:
    cmp word cx, 480
    je .end
.l2:
    add word dx, 1
    cmp word dx, 640
    je .l3
    int byte 0x20
    jmp word .l2
.l3:
    add word cx, 1
    jmp word .l1
.end:
    ret

gdt:
    .start:
    .null:
        dq 0
    .code:
        dw 0FFFFh
	    dw 0
	    db 0
	    db 10011010b
	    db 11001111b
	    db 0
    .data:
	    dw 0FFFFh
	    dw 0
	    db 0
	    db 10010010b
	    db 11001111b
	    db 0
    .end:
    
gdtr:
    dw gdt.end - gdt.start
    dd gdt.start