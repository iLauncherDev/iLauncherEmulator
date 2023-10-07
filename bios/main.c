#include <bios.h>

void main()
{
    outb(0xffff, 1 << 0);
    while (true)
        ;
}