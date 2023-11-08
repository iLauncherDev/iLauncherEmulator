#include <bios.h>
#include <io.h>
#include <bios_string.h>

void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
        outb(0x3f8, string[i]);
}

void main()
{
    char string_test[] = "iLauncherBios\n";
    while (true)
    {
        memset((void *)inl(0xfff0), 0xff, inw(0xfff4) * inw(0xfff6) * inw(0xfff8));
        memset((void *)inl(0xfff0), 0x00, inw(0xfff4) * inw(0xfff6) * inw(0xfff8));
    }
}
