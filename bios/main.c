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
        print(message);
    }
}