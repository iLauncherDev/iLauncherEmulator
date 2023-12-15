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
    if (cpu->code_packet[cpu->code_packet_index].completed)
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

void cpu_add_code_packet(cpu_t *cpu, uint8_t operantion, uint8_t sign,
                         uint8_t size, ...)
{
    if (!cpu)
        goto end;
    if (!cpu->code_packet[cpu->code_packet_index].completed)
    {
        cpu->code_packet_index++;
    }
end:
    return;
}

void cpu_execute(cpu_t *cpu)
{
    if (!cpu)
        goto end;
end:
    return;
}