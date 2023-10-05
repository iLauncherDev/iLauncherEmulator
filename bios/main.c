#include <bios.h>

void main()
{
    outb(0xffff, 0xff);
    uint8_t read = inb(0xffff);
    while (true)
        ;
}