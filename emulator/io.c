#include "io.h"

uint8_t io_ports[0x10000];

uint64_t io_read(uint16_t port, uint8_t size)
{
    return memory_read(&io_ports[port & 0xffff], size);
}

void io_write(uint16_t port, uint64_t value, uint8_t size)
{
    return memory_write(&io_ports[port & 0xffff], value, size);
}