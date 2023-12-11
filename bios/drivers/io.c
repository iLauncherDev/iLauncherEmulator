#include <io.h>

void outb(uint16_t port, uint8_t data)
{
    asm volatile("out %0, %1"
                 :
                 : "dN"(port), "a"(data));
}

uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("in %0, %1"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile("in %0, %1"
                 : "=a"(ret)
                 : "dN"(port));
    return ret;
}

void outw(uint16_t port, uint16_t data)
{
    asm volatile("out %0, %1"
                 :
                 : "dN"(port), "a"(data));
}

uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile("in %0, %1"
                 : "=a"(ret)
                 : "dN"(port));
    return ret;
}

void outl(uint16_t port, uint32_t data)
{
    asm volatile("out %0, %1"
                 :
                 : "dN"(port), "a"(data));
}
