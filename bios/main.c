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
    memcpy(string_test, (void *)0xf000, sizeof(string_test));
    while (true)
    {
        print((void *)0xf000);
    }
}
