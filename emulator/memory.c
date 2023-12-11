#include "memory.h"

memory_map_t *memory_map = (void *)NULL;

uint64_t memory_big_endian_read(void *ptr, uint8_t size)
{
    if (!ptr || !size)
        return 0;
    switch (size)
    {
    case 1:
        return (((uint8_t *)ptr)[0]);
    case 2:
        return ((uint16_t)(((uint8_t *)ptr)[0]) << 8) |
               (((uint8_t *)ptr)[1]);
    case 4:
        return ((uint32_t)(((uint8_t *)ptr)[0]) << 24) |
               ((uint32_t)(((uint8_t *)ptr)[1]) << 16) |
               ((uint16_t)(((uint8_t *)ptr)[2]) << 8) |
               (((uint8_t *)ptr)[3]);
    default:
        return ((uint64_t)(((uint8_t *)ptr)[0]) << 56) |
               ((uint64_t)(((uint8_t *)ptr)[1]) << 48) |
               ((uint64_t)(((uint8_t *)ptr)[2]) << 40) |
               ((uint64_t)(((uint8_t *)ptr)[3]) << 32) |
               ((uint32_t)(((uint8_t *)ptr)[4]) << 24) |
               ((uint32_t)(((uint8_t *)ptr)[5]) << 16) |
               ((uint16_t)(((uint8_t *)ptr)[6]) << 8) |
               (((uint8_t *)ptr)[7]);
    }
}

int64_t memory_big_endian_sread(void *ptr, uint8_t size)
{
    if (!ptr || !size)
        return 0;
    switch (size)
    {
    case 1:
        return (int8_t)(((uint8_t *)ptr)[0]);
    case 2:
        return (int16_t)(((uint16_t)(((uint8_t *)ptr)[0]) << 8) |
                         (((uint8_t *)ptr)[1]));
    case 4:
        return (int32_t)(((uint32_t)(((uint8_t *)ptr)[0]) << 24) |
                         ((uint32_t)(((uint8_t *)ptr)[1]) << 16) |
                         ((uint16_t)(((uint8_t *)ptr)[2]) << 8) |
                         (((uint8_t *)ptr)[3]));
    default:
        return (int64_t)(((uint64_t)(((uint8_t *)ptr)[0]) << 56) |
                         ((uint64_t)(((uint8_t *)ptr)[1]) << 48) |
                         ((uint64_t)(((uint8_t *)ptr)[2]) << 40) |
                         ((uint64_t)(((uint8_t *)ptr)[3]) << 32) |
                         ((uint32_t)(((uint8_t *)ptr)[4]) << 24) |
                         ((uint32_t)(((uint8_t *)ptr)[5]) << 16) |
                         ((uint16_t)(((uint8_t *)ptr)[6]) << 8) |
                         (((uint8_t *)ptr)[7]));
    }
}

void memory_big_endian_write(void *ptr, uint8_t size, uint64_t value)
{
    if (!ptr || !size)
        return;
    switch (size)
    {
    case 1:
        ((uint8_t *)ptr)[0] = value & 0xff;
        break;
    case 2:
        ((uint8_t *)ptr)[0] = (value >> 8) & 0xff;
        ((uint8_t *)ptr)[1] = value & 0xff;
        break;
    case 4:
        ((uint8_t *)ptr)[0] = (value >> 24) & 0xff;
        ((uint8_t *)ptr)[1] = (value >> 16) & 0xff;
        ((uint8_t *)ptr)[2] = (value >> 8) & 0xff;
        ((uint8_t *)ptr)[3] = value & 0xff;
        break;
    default:
        ((uint8_t *)ptr)[0] = (value >> 56) & 0xff;
        ((uint8_t *)ptr)[1] = (value >> 48) & 0xff;
        ((uint8_t *)ptr)[2] = (value >> 40) & 0xff;
        ((uint8_t *)ptr)[3] = (value >> 32) & 0xff;
        ((uint8_t *)ptr)[4] = (value >> 24) & 0xff;
        ((uint8_t *)ptr)[5] = (value >> 16) & 0xff;
        ((uint8_t *)ptr)[6] = (value >> 8) & 0xff;
        ((uint8_t *)ptr)[7] = value & 0xff;
        break;
    }
}

uint64_t memory_little_endian_read(void *ptr, uint8_t size)
{
    if (!ptr || !size)
        return 0;
    switch (size)
    {
    case 1:
        return (((uint8_t *)ptr)[0]);
    case 2:
        return (((uint8_t *)ptr)[0]) |
               ((uint16_t)(((uint8_t *)ptr)[1]) << 8);
    case 4:
        return (((uint8_t *)ptr)[0]) |
               ((uint16_t)(((uint8_t *)ptr)[1]) << 8) |
               ((uint32_t)(((uint8_t *)ptr)[2]) << 16) |
               ((uint32_t)(((uint8_t *)ptr)[3]) << 24);
    default:
        return (((uint8_t *)ptr)[0]) |
               ((uint16_t)(((uint8_t *)ptr)[1]) << 8) |
               ((uint32_t)(((uint8_t *)ptr)[2]) << 16) |
               ((uint32_t)(((uint8_t *)ptr)[3]) << 24) |
               ((uint64_t)(((uint8_t *)ptr)[4]) << 32) |
               ((uint64_t)(((uint8_t *)ptr)[5]) << 40) |
               ((uint64_t)(((uint8_t *)ptr)[6]) << 48) |
               ((uint64_t)(((uint8_t *)ptr)[7]) << 56);
    }
}

int64_t memory_little_endian_sread(void *ptr, uint8_t size)
{
    if (!ptr || !size)
        return 0;
    switch (size)
    {
    case 1:
        return (int8_t)(((uint8_t *)ptr)[0]);
    case 2:
        return (int16_t)((((uint8_t *)ptr)[0]) |
                         ((uint16_t)(((uint8_t *)ptr)[1]) << 8));
    case 4:
        return (int32_t)((((uint8_t *)ptr)[0]) |
                         ((uint16_t)(((uint8_t *)ptr)[1]) << 8) |
                         ((uint32_t)(((uint8_t *)ptr)[2]) << 16) |
                         ((uint32_t)(((uint8_t *)ptr)[3]) << 24));
    default:
        return (int64_t)((((uint8_t *)ptr)[0]) |
                         ((uint16_t)(((uint8_t *)ptr)[1]) << 8) |
                         ((uint32_t)(((uint8_t *)ptr)[2]) << 16) |
                         ((uint32_t)(((uint8_t *)ptr)[3]) << 24) |
                         ((uint64_t)(((uint8_t *)ptr)[4]) << 32) |
                         ((uint64_t)(((uint8_t *)ptr)[5]) << 40) |
                         ((uint64_t)(((uint8_t *)ptr)[6]) << 48) |
                         ((uint64_t)(((uint8_t *)ptr)[7]) << 56));
    }
}

void memory_little_endian_write(void *ptr, uint8_t size, uint64_t value)
{
    if (!ptr || !size)
        return;
    switch (size)
    {
    case 1:
        ((uint8_t *)ptr)[0] = value & 0xff;
        break;
    case 2:
        ((uint8_t *)ptr)[0] = value & 0xff;
        ((uint8_t *)ptr)[1] = (value >> 8) & 0xff;
        break;
    case 4:
        ((uint8_t *)ptr)[0] = value & 0xff;
        ((uint8_t *)ptr)[1] = (value >> 8) & 0xff;
        ((uint8_t *)ptr)[2] = (value >> 16) & 0xff;
        ((uint8_t *)ptr)[3] = (value >> 24) & 0xff;
        break;
    default:
        ((uint8_t *)ptr)[0] = value & 0xff;
        ((uint8_t *)ptr)[1] = (value >> 8) & 0xff;
        ((uint8_t *)ptr)[2] = (value >> 16) & 0xff;
        ((uint8_t *)ptr)[3] = (value >> 24) & 0xff;
        ((uint8_t *)ptr)[4] = (value >> 32) & 0xff;
        ((uint8_t *)ptr)[5] = (value >> 40) & 0xff;
        ((uint8_t *)ptr)[6] = (value >> 48) & 0xff;
        ((uint8_t *)ptr)[7] = (value >> 56) & 0xff;
        break;
    }
}

uint64_t memory_read(uint64_t address, uint8_t size, uint8_t big_endian)
{
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                if (address - tmp->address + size > tmp->size)
                {
                    tmp = (void *)NULL;
                    break;
                }
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
        if (address + size - 1 >= vm_memory_size)
            goto end;
    }
    if (big_endian)
        return memory_big_endian_read(&buffer[address], size);
    else
        return memory_little_endian_read(&buffer[address], size);
end:
    return 0x00;
}

int64_t memory_sread(uint64_t address, uint8_t size, uint8_t big_endian)
{
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address <= tmp->address + tmp->size)
            {
                if (address - tmp->address + size > tmp->size)
                {
                    tmp = (void *)NULL;
                    break;
                }
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
        if (address + size - 1 >= vm_memory_size)
            goto end;
    }
    if (big_endian)
        return memory_big_endian_sread(&buffer[address], size);
    else
        return memory_little_endian_sread(&buffer[address], size);
end:
    return 0x00;
}

void memory_write(uint64_t address, uint64_t value, uint8_t size, uint8_t big_endian)
{
    uint8_t *buffer = vm_memory;
    memory_map_t *tmp = memory_map;
    if (memory_map)
    {
        while (tmp)
        {
            if (address >= tmp->address && address < tmp->address + tmp->size)
            {
                if (address - tmp->address + size > tmp->size)
                {
                    tmp = (void *)NULL;
                    break;
                }
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
        if (address + size - 1 >= vm_memory_size)
            goto end;
    }
    if (big_endian)
        return memory_big_endian_write(&buffer[address], size, value);
    else
        return memory_little_endian_write(&buffer[address], size, value);
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