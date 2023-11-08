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
    memcpy((void *)0xf000, string_test, sizeof(string_test));
    while (true)
    {
        print(string_test);
    }
}
