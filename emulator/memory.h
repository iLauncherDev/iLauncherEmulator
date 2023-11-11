#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include "global.h"
#define MEMORY_READ_FLAG (1 << 0)
#define MEMORY_WRITE_FLAG (1 << 1)
#define MEMORY_RAM_FLAG (1 << 2)

typedef struct memory_map
{
    uint8_t flags;
    void *buffer;
    uint64_t address, offset, size;
    struct memory_map *prev;
    struct memory_map *next;
} memory_map_t;

uint64_t memory_big_endian_read(void *ptr, uint8_t size);
void memory_big_endian_write(void *ptr, uint8_t size, uint64_t value);
uint64_t memory_little_endian_read(void *ptr, uint8_t size);
void memory_little_endian_write(void *ptr, uint8_t size, uint64_t value);
uint64_t memory_read(uint64_t address, uint8_t size, uint8_t big_endian);
void memory_write(uint64_t address, uint64_t value, uint8_t size, uint8_t big_endian);
void memory_map_buffer(uint8_t flags, void *buffer, uint64_t address, uint64_t offset, uint64_t size);
void memory_map_set_offset(uint64_t address, uint64_t offset);
void memory_map_remove(uint64_t address);
#endif