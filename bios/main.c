#include <bios.h>
#include <io.h>

void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
        outb(0xffff, string[i]);
}

void main()
{
    char message[] = "iLauncherBios\n";
    while (true)
        print(message);
}