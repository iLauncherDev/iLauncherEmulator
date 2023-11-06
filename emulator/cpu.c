#include "cpu.h"

global_uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg)
{
    if (!cpu)
        goto end;
    if (cpu->read_reg)
        return cpu->read_reg(cpu, reg);
end:
    return 0;
}

void cpu_write_reg(cpu_t *cpu, uint16_t reg, global_uint64_t value)
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
    {
        if (cpu->emulate(cpu))
            printf("Unknown opcode: 0x%llx\n", memory_read(cpu->pc - 1, 1, 0));
    }
    cpu->info_index = 0;
end:
    return;
}