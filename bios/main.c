#include <bios.h>
#include <io.h>

void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
        outb(0x3f8, string[i]);
}

void main()
{
    while (true)
    {
        *(uint32_t *)0xf000 = 'YAKO';
        print((char *)0xf000);
        *(uint32_t *)0xf000 = '\n';
        print((char *)0xf000);
    }
}