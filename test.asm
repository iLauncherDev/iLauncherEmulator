bits 16

loop:
    add ax, 0x01
    int 0x20
    jmp loop