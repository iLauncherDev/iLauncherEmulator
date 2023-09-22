#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
#include <stdbool.h>

extern uint64_t vm_memory_size, bios_size;
extern uint8_t *vm_memory;

static inline uint64_t memory_read(void *address, uint8_t size)
{
    switch (size)
    {
    case 1:
        return *((uint8_t *)address);
    case 2:
        return *((uint16_t *)address);
    case 4:
        return *((uint32_t *)address);
    default:
        return *((uint64_t *)address);
    }
}

static inline void memory_write(void *address, uint64_t value, uint8_t size)
{
    switch (size)
    {
    case 1:
        *((uint8_t *)address) = (uint8_t)value;
        break;
    case 2:
        *((uint16_t *)address) = (uint16_t)value;
        break;
    case 4:
        *((uint32_t *)address) = (uint32_t)value;
        break;
    default:
        *((uint64_t *)address) = (uint64_t)value;
        break;
    }
}

#endif