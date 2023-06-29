bits 16

loop:
    add ax, 1
    int 0x20
    jmp loop
