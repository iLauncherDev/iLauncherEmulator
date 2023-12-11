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

int64_t cpu_sread_reg(cpu_t *cpu, uint16_t reg)
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
    if (cpu->code_packet[cpu->code_packet_index].completed)
        goto end;
    if (cpu->flags & cpu_flag_jump)
    {
        cpu->pc = cpu->pc_new;
        cpu->pc_base = cpu->pc_base_new;
        cpu->flags &= ~cpu_flag_jump;
    }
    while (cpu->flags & cpu_flag_lock)
        ;
    if (cpu->flags & cpu_flag_reset)
    {
        if (cpu->reset)
            cpu->reset(cpu);
        cpu->flags &= ~cpu_flag_reset;
    }
    if (cpu->emulate)
    {
        if (cpu->emulate(cpu))
            printf("Unknown opcode: 0x%" PRIx64 "\n", memory_read(cpu->pc_base + cpu->pc, 1, 0)), cpu->pc++;
    }
end:
    return;
}

void cpu_add_code_packet(cpu_t *cpu, uint8_t operantion, uint8_t sign, uint8_t type,
                         uint64_t reg_x, uint64_t reg_y, uint8_t size)
{
    if (!cpu)
        goto end;
    if (!cpu->code_packet[cpu->code_packet_index].completed)
    {
        cpu->code_packet[cpu->code_packet_index].operation = operantion;
        cpu->code_packet[cpu->code_packet_index].sign = sign;
        cpu->code_packet[cpu->code_packet_index].type = type;
        cpu->code_packet[cpu->code_packet_index].reg_x = reg_x;
        cpu->code_packet[cpu->code_packet_index].reg_y = reg_y;
        cpu->code_packet[cpu->code_packet_index].size = size;
        cpu->code_packet[cpu->code_packet_index].completed = 1;
        cpu->code_packet_index++;
    }
end:
    return;
}

void cpu_execute_packet(cpu_t *cpu)
{
    if (!cpu)
        goto end;
    if (!cpu->code_packet[cpu->exec_code_packet_index].completed)
    {
        cpu->exec_code_packet_index = 0;
        goto end;
    }
    if (cpu->execute_packet)
        cpu->execute_packet(cpu);
    cpu->exec_code_packet_index++;
end:
    return;
}