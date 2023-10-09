#include <bios.h>

void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
        outb(0xffff, string[i]);
}

void main()
{
    char message[] = "Hello World\n";
    outb(0xffff, 'H');
    outb(0xffff, 'e');
    outb(0xffff, 'l');
    outb(0xffff, 'l');
    outb(0xffff, 'o');
    outb(0xffff, ' ');
    outb(0xffff, 'W');
    outb(0xffff, 'o');
    outb(0xffff, 'r');
    outb(0xffff, 'l');
    outb(0xffff, 'd');
    outb(0xffff, '\n');
    while (true)
        ;
}