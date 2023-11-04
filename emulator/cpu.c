#include "cpu.h"

global_uint64_t cpu_read_reg(cpu_t *cpu, uint8_t reg)
{
    if (!cpu)
        goto end;
    if (cpu->read_reg)
        return cpu->read_reg(cpu, reg);
end:
    return 0;
}

void cpu_write_reg(cpu_t *cpu, uint8_t reg, global_uint64_t value)
{
    if (!cpu)
        goto end;
    if (cpu->write_reg)
        return cpu->write_reg(cpu, reg, value);
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

void cpu_emulate(cpu_t *cpu)
{
    if (!cpu)
        goto end;
    if (cpu->emulate)
        cpu->emulate(cpu);
    cpu->info_index = 0;
end:
    return;
}