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
    push word ax
    mov word dx, [0xf4f5]
    mov word ax, sp
    mov word sp, 0x00
    mov word sp, ax
    pop word ax
    mov word bp, 0x00
    mov word bx, 0x00
    mov word cx, 0x00
    mov word dx, 0x00
    cmp word ax, 0x00
    je loop
    jmp start

loop:use16
    mov cx, 0x0000
    mov dx, 0x0000
.l1:
    cmp cx, 480
    je .end
.l2:
    int byte 0x20
    add dx, 1
    cmp dx, 640
    je .l3
    jmp .l2
.l3:
    add cx, 1
    jmp .l1
.end:
    add ax, 0x3f
    add bx, 0x3f
    jmp loop

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