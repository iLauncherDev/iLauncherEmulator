#include "cpu.h"

uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg)
{
    if (!cpu)
        goto end;
    if (cpu->read_reg)
        return cpu->read_reg(cpu, reg);
end:
    return 0;
}

void cpu_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value)
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
    while (cpu->flags & cpu_flag_wait)
        ;
    cpu->flags |= cpu_flag_emulating;
    if (cpu->emulate)
    {
        if (cpu->emulate(cpu))
            printf("Unknown opcode: 0x%lx\n", memory_read(cpu->pc_base + cpu->pc, 1, 0)), cpu->pc++;
    }
    cpu->flags &= ~cpu_flag_emulating;
end:
    return;
}