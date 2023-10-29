#include <bios_string.h>

void memcpy(void *dest, void *src, uint32_t size)
{
    while (size--)
        *(uint8_t *)dest++ = *(uint8_t *)src++;
}

void memset(void *dest, uint8_t value, uint32_t size)
{
    while (size--)
        *(uint8_t *)dest++ = value;
}