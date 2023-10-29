#include "memory.h"

memory_map_t *memory_map = (void *)NULL;

global_uint64_t memory_big_endian_read(void *ptr, uint8_t bits)
{
    if (!ptr || !bits)
        goto end;
    global_uint64_t value = 0;
    for (uint8_t i = bits; i > 8; i -= 8)
        value |= *(uint8_t *)ptr++ << (i - 8);
end:
    return value;
}

void memory_big_endian_write(void *ptr, uint8_t bits, global_uint64_t value)
{
    if (!ptr || !bits)
        goto end;
    for (uint8_t i = bits; i > 8; i -= 8)
        *(uint8_t *)ptr++ = value >> (i - 8);
end:
    return;
}

global_uint64_t memory_little_endian_read(void *ptr, uint8_t bits)
{
    if (!ptr || !bits)
        goto end;
    global_uint64_t value = 0;
    for (uint8_t i = 0; i < bits; i += 8)
        value |= *(uint8_t *)ptr++ << i;
end:
    return value;
}

void memory_little_endian_write(void *ptr, uint8_t bits, global_uint64_t value)
{
    if (!ptr || !bits)
        goto end;
    for (uint8_t i = 0; i < bits; i += 8)
        *(uint8_t *)ptr++ = value >> i;
end:
    return;
}

global_uint64_t memory_read(global_uint64_t address, uint8_t size, uint8_t big_endian)
{
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                buffer = tmp->buffer;
                address = (address - tmp->address) + tmp->offset;
            }
            tmp = tmp->next;
        }
    }
    if (tmp)
    {
        if (~tmp->flags & MEMORY_READ_FLAG)
            goto end;
    }
    else
    {
        if (address >= vm_memory_size)
            goto end;
    }
    if (big_endian)
        return memory_big_endian_read(&buffer[address], size << 3);
    else
        return memory_little_endian_read(&buffer[address], size << 3);
end:
    return 0x00;
}

void memory_write(global_uint64_t address, global_uint64_t value, uint8_t size, uint8_t big_endian)
{
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address < tmp->address + tmp->size)
            {
                buffer = tmp->buffer;
                address = (address - tmp->address) + tmp->offset;
            }
            tmp = tmp->next;
        }
    }
    if (tmp)
    {
        if (~tmp->flags & MEMORY_WRITE_FLAG)
            goto end;
    }
    else
    {
        if (address >= vm_memory_size)
            goto end;
    }
    if (big_endian)
        return memory_big_endian_write(&buffer[address], size << 3, value);
    else
        return memory_little_endian_write(&buffer[address], size << 3, value);
end:
    return;
}

void memory_map_set_offset(global_uint64_t address, global_uint64_t offset)
{
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                tmp->offset = offset & (tmp->size - 1);
                break;
            }
            tmp = tmp->next;
        }
    }
}

void memory_map_remove(global_uint64_t address)
{
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                if (tmp->prev)
                {
                    tmp->prev = tmp->next;
                    tmp->next->prev = tmp->prev;
                    free(tmp);
                }
                else
                {
                    if (tmp->next)
                    {
                        tmp->next->prev = (void *)NULL;
                    }
                    else
                    {
                        free(tmp);
                    }
                }
                break;
            }
            tmp = tmp->next;
        }
    }
}

void memory_map_buffer(uint8_t flags, void *buffer, global_uint64_t address, global_uint64_t offset, global_uint64_t size)
{
    if (memory_map)
    {
        memory_map_t *tmp = memory_map;
        while (tmp->next)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
                return;
            tmp = tmp->next;
        }
        tmp->next = (void *)malloc(sizeof(memory_map_t));
        memset(tmp->next, 0, sizeof(memory_map_t));
        tmp->next->prev = tmp;
        tmp->next->flags = flags;
        tmp->next->buffer = buffer;
        tmp->next->offset = offset;
        tmp->next->address = address;
        tmp->next->size = size;
    }
    else
    {
        memory_map = (void *)malloc(sizeof(memory_map_t));
        memset(memory_map, 0, sizeof(memory_map_t));
        memory_map->flags = flags;
        memory_map->buffer = buffer;
        memory_map->offset = offset;
        memory_map->address = address;
        memory_map->size = size;
    }
}