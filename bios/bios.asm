bits 16
section .text
extern main

start:
    mov esp, 0x3000
    call main
.loop:
    jmp .loop

bits 16
section .null
reset:
    cli
    cld
    jmp 0xe000:0x00

times 0x0f - ($ - $$) db 0x00