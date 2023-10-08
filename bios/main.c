#include <bios.h>

void main()
{
    while (true)
        outb(0xffff, inb(0xffff) + 1);
}