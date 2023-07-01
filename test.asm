bits 16

start:
    mov ax, 0x00

loop:
    add ax, 0x01
    int 0x20
    add bx, 0x01
    jmp loop