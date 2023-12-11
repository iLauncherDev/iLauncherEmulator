#include <bios.h>
#include <io.h>
#include <bios_string.h>

void print(char *string)
{
    for (uint32_t i = 0; string[i]; i++)
    {
        outb(0x3f8, string[i]);
        while (inb(0x3f8))
            ;
    }
}

void putpixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint16_t width = inw(0xfff4), height = inw(0xfff6);
    if (x >= width)
        x = width;
    if (y >= height)
        y = height;
    switch (inb(0xfff8))
    {
    case 1:
        ((uint8_t *)inl(0xfff0))[y * width + x] = color;
        break;
    case 2:
        ((uint16_t *)inl(0xfff0))[y * width + x] = color;
        break;
    case 4:
        ((uint32_t *)inl(0xfff0))[y * width + x] = color;
        break;
    }
}

void main()
{
    void *buffer = (void *)inl(0xfff0);
    uint32_t size = inw(0xfff4) * inw(0xfff6) * inb(0xfff8);
    uint8_t bpp = inb(0xfff8);
    char string_test[] = "iLauncherBios\n";
    memcpy((void *)0xf000, string_test, sizeof(string_test));
    while (true)
    {
        memset(buffer, 0xff, size);
        memset(buffer, 0x00, size);
        print((void *)0xf000);
    }
}
