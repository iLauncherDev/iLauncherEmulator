#include <bios.h>

void main()
{
    outb(0xffff, inb(0xffff) | (1 << 2));
    outb(0xffff, inb(0xffff) | (1 << 1));
    while (true)
        ;
}