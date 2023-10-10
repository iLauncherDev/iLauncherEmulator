#include "memory.h"

memory_map_t *memory_map = (void *)NULL;

uint64_t memory_read(uint64_t address, uint8_t size)
{
    if (address >= vm_memory_size)
        return 0x00000000;
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                buffer = tmp->buffer;
                address = (address - tmp->address) + (tmp->offset & (tmp->size - 1));
                break;
            }
            tmp = tmp->next;
        }
    }
    if (tmp)
        if (~tmp->flags & MEMORY_READ_FLAG)
            goto end;
    switch (size)
    {
    case 1:
        return *((uint8_t *)&buffer[address]);
    case 2:
        return *((uint16_t *)&buffer[address]);
    case 4:
        return *((uint32_t *)&buffer[address]);
    default:
        return *((uint64_t *)&buffer[address]);
    }
end:
    return 0x00;
}

void memory_write(uint64_t address, uint64_t value, uint8_t size)
{
    if (address >= vm_memory_size)
        return;
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                buffer = tmp->buffer;
                address = (address - tmp->address) + (tmp->offset & (tmp->size - 1));
                break;
            }
            tmp = tmp->next;
        }
    }
    if (tmp)
        if (~tmp->flags & MEMORY_WRITE_FLAG)
            goto end;
    switch (size)
    {
    case 1:
        *((uint8_t *)&buffer[address]) = (uint8_t)value;
        break;
    case 2:
        *((uint16_t *)&buffer[address]) = (uint16_t)value;
        break;
    case 4:
        *((uint32_t *)&buffer[address]) = (uint32_t)value;
        break;
    default:
        *((uint64_t *)&buffer[address]) = (uint64_t)value;
        break;
    }
end:
    return;
}

void memory_map_set_offset(uint64_t address, uint64_t offset)
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

void memory_map_remove(uint64_t address)
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

void memory_map_buffer(uint8_t flags, void *buffer, uint64_t address, uint64_t offset, uint64_t size)
{
    if (memory_map)
    {
        memory_map_t *tmp = memory_map;
        while (tmp->next)
            tmp = tmp->next;
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