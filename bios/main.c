#include <bios.h>

static inline void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
        outb(0xffff, string[i]);
}

void main()
{
    char message[] = "Hello World\n";
    while (true)
    {
        outb(0xffff, 'i');
        outb(0xffff, 'L');
        outb(0xffff, 'a');
        outb(0xffff, 'u');
        outb(0xffff, 'n');
        outb(0xffff, 'c');
        outb(0xffff, 'h');
        outb(0xffff, 'e');
        outb(0xffff, 'r');
        outb(0xffff, ' ');
        outb(0xffff, 'B');
        outb(0xffff, 'i');
        outb(0xffff, 'o');
        outb(0xffff, 's');
        outb(0xffff, '\n');
    }
}