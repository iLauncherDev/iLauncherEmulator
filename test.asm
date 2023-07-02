bits 16

start:
    mov ax, 0x00
    mov bx, 0x00
    mov cx, 0x00
    mov dx, 0x00
    mov sp, stack_end

loop:
    add ax, 0x01
    add bx, 0x02
    add cx, 0x03
    int 0x20
    add dx, 0x01
    jmp loop

stack_start:
    times 0x1000 db 0x0000
stack_end: