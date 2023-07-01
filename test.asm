bits 16

start:
    mov ax, 0x00
    mov bx, 0x00
    mov cx, 0x00
    mov dx, 0x00

loop:
    add ax, 0x01
    add bx, 0x02
    add cx, 0x03
    int 0x20
    add dx, 0x01
    jmp loop