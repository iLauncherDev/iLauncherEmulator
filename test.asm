bits 16
section .bss

stack_start:
    resb 0x1000
stack_end:

section .text

test:
    jmp start
    lgdt [gdtr]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
	jmp	0x08:.flush
.flush:use32
	jmp dword .flush

start:use16
    mov word sp, stack_end
    push word ax
    mov word dx, [0x00]
    mov word ax, sp
    mov word sp, 0x00
    mov word sp, ax
    pop word ax
    mov word bp, 0x00
    mov word bx, 0x00
    mov word cx, 0x00
    mov word dx, 0x00

loop:use16
    int byte 0x20
    add word ax, 0x01
    jmp word loop

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