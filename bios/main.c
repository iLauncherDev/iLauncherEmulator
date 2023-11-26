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
    uint32_t *buffer = (void *)inl(0xfff0);
    uint32_t size = inw(0xfff4) * inw(0xfff6);
    char string_test[] = "iLauncherBios\n";
    memcpy((void *)0xf000, string_test, sizeof(string_test));
    while (true)
    {
        for (uint32_t i = 0; i < size; i++)
            buffer[i] = 0xffffffff;
        for (uint32_t i = 0; i < size; i++)
            buffer[i] = 0x00000000;
        print((void *)0xf000);
    }
}
