#include <bios.h>

void main()
{
    while (true)
    {
        *(uint16_t *)0xf0000 = 0xffff;
    }
}