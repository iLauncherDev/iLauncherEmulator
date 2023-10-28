#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include "global.h"
#define MEMORY_READ_FLAG (1 << 0)
#define MEMORY_WRITE_FLAG (1 << 1)

typedef struct memory_map
{
    uint8_t flags;
    void *buffer;
    uint64_t address, offset, size;
    struct memory_map *prev;
    struct memory_map *next;
} memory_map_t;

void *memory_read_address(uint64_t address);
uint64_t memory_read(uint64_t address, uint8_t size);
void memory_write(uint64_t address, uint64_t value, uint8_t size);
void memory_map_buffer(uint8_t flags, void *buffer, uint64_t address, uint64_t offset, uint64_t size);
void memory_map_set_offset(uint64_t address, uint64_t offset);
void memory_map_remove(uint64_t address);
#endif