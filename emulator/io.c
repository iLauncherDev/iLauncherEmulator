#include "io.h"

uint8_t io_ports[0x20000];

uint8_t io_check_flag(uint16_t port, uint8_t flag, uint8_t size)
{
    uint8_t bits = size << 3, bits_output = 0;
    uint32_t port_offset = (port & 0xffff) << 1;
    for (uint8_t i = 0; i < bits; i += 8)
    {
        if (io_ports[port_offset + 1] & flag)
            bits_output += 8;
        port_offset += 2;
    }
    return bits == bits_output ? 1 : 0;
}

void io_clear_flag(uint16_t port, uint8_t flag, uint8_t size)
{
    uint8_t bits = size << 3;
    uint32_t port_offset = (port & 0xffff) << 1;
    for (uint8_t i = 0; i < bits; i += 8)
    {
        io_ports[port_offset + 1] &= ~flag;
        port_offset += 2;
    }
}

global_uint64_t io_read(uint16_t port, uint8_t size)
{
    uint8_t bits = size << 3;
    uint32_t port_offset = (port & 0xffff) << 1;
    global_uint64_t value = 0;
    for (uint8_t i = 0; i < bits; i += 8)
    {
        value |= io_ports[port_offset] << i;
        io_ports[port_offset + 1] |= IO_READ_FLAG;
        port_offset += 2;
    }
    return value;
}

void io_write(uint16_t port, global_uint64_t value, uint8_t size)
{
    uint8_t bits = size << 3;
    uint32_t port_offset = (port & 0xffff) << 1;
    for (uint8_t i = 0; i < bits; i += 8)
    {
        io_ports[port_offset] = value >> i;
        io_ports[port_offset + 1] |= IO_WRITE_FLAG;
        port_offset += 2;
    }
}