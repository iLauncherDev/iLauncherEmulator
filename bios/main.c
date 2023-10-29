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
    char message[] = "iLauncherBios\n";
    memcpy((void *)0xf000, message, sizeof(message));
    while (true)
    {
        print((char *)0xf000);
    }
}
