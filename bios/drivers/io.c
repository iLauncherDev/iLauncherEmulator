#include <io.h>

void outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %1, %0"
                 :
                 : "dN"(port), "a"(data));
}

uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile("inw %1, %0"
                 : "=a"(ret)
                 : "dN"(port));
    return ret;
}

void outw(uint16_t port, uint16_t data)
{
    asm volatile("outw %1, %0"
                 :
                 : "dN"(port), "a"(data));
}
