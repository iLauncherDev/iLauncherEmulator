#include <bios.h>
#include <io.h>
#include <bios_string.h>

void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
        outb(0x3f8, string[i]);
}

uint32_t multi(uint32_t a, uint32_t b)
{
    uint32_t ret = 0;
    while (b--)
        ret += a;
    return ret;
}

void main()
{
    while (true)
    {
        for (uint16_t y = 0; y < inw(0xfff6); y++)
            memset((void *)(inl(0xfff0) + multi(inw(0xfff4), y)), 0xff, inw(0xfff4) << 2);
    }
}
