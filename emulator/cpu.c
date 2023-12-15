#include "cpu.h"

uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    if (!cpu)
        goto end;
    if (cpu->read_reg)
        return cpu->read_reg(cpu, reg, size);
end:
    return 0;
}

int64_t cpu_sread_reg(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    if (!cpu)
        goto end;
    if (cpu->read_reg)
        return cpu->read_reg(cpu, reg, size);
end:
    return 0;
}

void cpu_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value, uint8_t size)
{
    if (!cpu)
        goto end;
    if (cpu->write_reg)
        return cpu->write_reg(cpu, reg, value, size);
end:
    return;
}

void cpu_reset(cpu_t *cpu)
{
    if (!cpu)
        goto end;
    if (cpu->reset)
        cpu->reset(cpu);
end:
    return;
}

void cpu_recompile(cpu_t *cpu)
{
    if (!cpu)
        goto end;
    while (cpu->flags & cpu_flag_lock)
        ;
    if (cpu->flags & cpu_flag_reset)
    {
        if (cpu->reset)
            cpu->reset(cpu);
        cpu->flags &= ~cpu_flag_reset;
    }
    if (cpu->recompile)
        while (cpu->recompile(cpu))
            ;
end:
    return;
}

uint64_t cpu_packet_read(cpu_t *cpu, cpu_packet_t *packet, uint8_t index)
{
    switch (packet->values[index].type)
    {
    case cpu_type_int:
        return packet->values[index].value;
    case cpu_type_reg:
        return cpu_read_reg(cpu, packet->values[index].value, packet->values[index].size);
    case cpu_type_mem:
        return memory_read(packet->values[index].value, packet->values[index].size, cpu->big_endian);
    }
    return 0;
}

int64_t cpu_packet_sread(cpu_t *cpu, cpu_packet_t *packet, uint8_t index)
{
    switch (packet->values[index].type)
    {
    case cpu_type_int:
        return (int64_t)packet->values[index].value;
    case cpu_type_reg:
        return cpu_sread_reg(cpu, packet->values[index].value, packet->values[index].size);
    case cpu_type_mem:
        return memory_sread(packet->values[index].value, packet->values[index].size, cpu->big_endian);
    }
    return 0;
}

void cpu_packet_write(cpu_t *cpu, cpu_packet_t *packet, uint64_t value, uint8_t index)
{
    switch (packet->values[index].type)
    {
    case cpu_type_int:
        return;
    case cpu_type_reg:
        cpu_write_reg(cpu, packet->values[index].value, value, packet->values[index].size);
        return;
    case cpu_type_mem:
        memory_write(packet->values[index].value, value, packet->values[index].size, cpu->big_endian);
        return;
    }
}

void cpu_execute(cpu_t *cpu)
{
    if (!cpu)
        goto end;
    for (uint16_t i = 0; i < 4096; i++)
    {
        cpu_packet_t *packet = &cpu->code_packet[i];
        switch (packet->instruction)
        {
        case cpu_opcode_quit:
            goto end;
        case cpu_opcode_mov:
            cpu_packet_write(cpu, packet, cpu_packet_read(cpu, packet, 1), 0);
            break;
        }
        cpu->pc += packet->opcode_size;
    }
end:
    return;
}