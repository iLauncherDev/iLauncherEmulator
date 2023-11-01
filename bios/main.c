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
    char test[] = "iLauncherBios\n";
    print(test);
}
