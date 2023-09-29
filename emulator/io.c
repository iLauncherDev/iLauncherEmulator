#include "io.h"

uint8_t io_ports[0x10000];

uint64_t io_read(uint16_t port, uint8_t size)
{
    switch (size)
    {
    case 1:
        return *((uint8_t *)&io_ports[port & 0xffff]);
    case 2:
        return *((uint16_t *)&io_ports[port & 0xffff]);
    case 4:
        return *((uint32_t *)&io_ports[port & 0xffff]);
    default:
        return *((uint64_t *)&io_ports[port & 0xffff]);
    }
}

void io_write(uint16_t port, uint64_t value, uint8_t size)
{
    switch (size)
    {
    case 1:
        *((uint8_t *)&io_ports[port & 0xffff]) = value;
    case 2:
        *((uint16_t *)&io_ports[port & 0xffff]) = value;
    case 4:
        *((uint32_t *)&io_ports[port & 0xffff]) = value;
    default:
        *((uint64_t *)&io_ports[port & 0xffff]) = value;
    }
}