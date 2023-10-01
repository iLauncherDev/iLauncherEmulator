#include <bios.h>

void main()
{
    outb(0xffff, 0xff);
    while (true)
        ;
}