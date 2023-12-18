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
    {
        for (cpu->code_block_index = 0;
             cpu->code_block_index < CPU_BLOCK_SIZE;
             cpu->code_block_index++)
        {
            uint16_t index = cpu->code_block_index;
            uint64_t pc = cpu->pc;
            cpu->code_block[index].pc = pc;
            if (cpu->recompile(cpu))
                break;
            cpu->code_block[index].opcode_size = cpu->pc - pc;
        }
    }
    cpu->flags &= ~cpu_flag_stoped;
end:
    return;
}

void cpu_block_add(cpu_t *cpu, uint8_t instruction, uint8_t sign, uint8_t value_length, ...)
{
    va_list args;
    uint16_t index;
    if (!cpu)
        goto end;
    va_start(args, value_length);
    index = cpu->code_block_index;
    cpu->code_block[index].instruction = instruction;
    cpu->code_block[index].sign = sign;
    cpu->code_block[index].value_length = value_length & 3;
    for (uint8_t i = 0; i < cpu->code_block[index].value_length; i++)
    {
        cpu->code_block[index].values[i].type = va_arg(args, uint8_t);
        cpu->code_block[index].values[i].size = va_arg(args, uint8_t);
        cpu->code_block[index].values[i].value = va_arg(args, uint64_t);
        cpu->code_block[index].values[i].offset = va_arg(args, int64_t);
    }
end:
    return;
}

uint64_t cpu_block_address(cpu_t *cpu, cpu_block_t *packet, uint8_t index)
{
    switch (packet->values[index].type)
    {
    case cpu_type_int:
        return packet->values[index].value;
    case cpu_type_reg:
        return cpu_read_reg(cpu, packet->values[index].value, packet->values[index].size);
    case cpu_type_mem:
        return packet->values[index].value;
    case cpu_type_mreg:
        return cpu_read_reg(cpu,
                            (packet->values[index].value >> 0) & 0xffff,
                            packet->values[index].size) +
               cpu_read_reg(cpu,
                            (packet->values[index].value >> 16) & 0xffff,
                            packet->values[index].size) *
                   ((packet->values[index].value >> 32) & 0xffffffff) +
               packet->values[index].offset;
    }
    return 0;
}

uint64_t cpu_block_read(cpu_t *cpu, cpu_block_t *packet, uint8_t index)
{
    switch (packet->values[index].type)
    {
    case cpu_type_int:
        return packet->values[index].value;
    case cpu_type_reg:
        return cpu_read_reg(cpu, packet->values[index].value, packet->values[index].size);
    case cpu_type_mem:
        return memory_read(packet->values[index].value, packet->values[index].size, cpu->big_endian);
    case cpu_type_mreg:
        return memory_read(cpu_read_reg(cpu,
                                        (packet->values[index].value >> 0) & 0xffff,
                                        packet->values[index].size) +
                               cpu_read_reg(cpu,
                                            (packet->values[index].value >> 16) & 0xffff,
                                            packet->values[index].size) *
                                   ((packet->values[index].value >> 32) & 0xffffffff) +
                               packet->values[index].offset,
                           packet->values[index].size, cpu->big_endian);
    }
    return 0;
}

int64_t cpu_block_sread(cpu_t *cpu, cpu_block_t *packet, uint8_t index)
{
    switch (packet->values[index].type)
    {
    case cpu_type_int:
        return (int64_t)packet->values[index].value;
    case cpu_type_reg:
        return cpu_sread_reg(cpu, packet->values[index].value, packet->values[index].size);
    case cpu_type_mem:
        return memory_sread(packet->values[index].value, packet->values[index].size, cpu->big_endian);
    case cpu_type_mreg:
        return memory_sread(cpu_read_reg(cpu,
                                         (packet->values[index].value >> 0) & 0xffff,
                                         packet->values[index].size) +
                                cpu_read_reg(cpu,
                                             (packet->values[index].value >> 16) & 0xffff,
                                             packet->values[index].size) *
                                    ((packet->values[index].value >> 32) & 0xffffffff) +
                                packet->values[index].offset,
                            packet->values[index].size, cpu->big_endian);
    }
    return 0;
}

void cpu_block_write(cpu_t *cpu, cpu_block_t *packet, uint64_t value, uint8_t index)
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
    case cpu_type_mreg:
        memory_write(cpu_read_reg(cpu,
                                  (packet->values[index].value >> 0) & 0xffff,
                                  packet->values[index].size) +
                         cpu_read_reg(cpu,
                                      (packet->values[index].value >> 16) & 0xffff,
                                      packet->values[index].size) *
                             ((packet->values[index].value >> 32) & 0xffffffff) +
                         packet->values[index].offset,
                     value,
                     packet->values[index].size, cpu->big_endian);
        return;
    }
}

void cpu_execute(cpu_t *cpu)
{
    if (!cpu || cpu->flags & cpu_flag_stoped)
        goto end;
    uint16_t i = 0;
    uint64_t cache[4];
    bool okay = false;
    while (i < CPU_BLOCK_SIZE)
    {
        cpu_block_t *packet = &cpu->code_block[i];
        switch (packet->instruction)
        {
        case cpu_opcode_quit:
            goto end;
        case cpu_opcode_cmp:
            cache[2] = cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size);
            if (packet->sign)
            {
                cache[0] = cpu_block_sread(cpu, packet, 0);
                cache[1] = cpu_block_sread(cpu, packet, 1);
                if ((int64_t)cache[0] < (int64_t)cache[1])
                    cache[2] |= cpu->neutral_values[cpu_neutral_flag_carry];
                else
                    cache[2] &= ~cpu->neutral_values[cpu_neutral_flag_carry];
            }
            else
            {
                cache[0] = cpu_block_read(cpu, packet, 0);
                cache[1] = cpu_block_read(cpu, packet, 1);
                if (cache[0] < cache[1])
                    cache[2] |= cpu->neutral_values[cpu_neutral_flag_carry];
                else
                    cache[2] &= ~cpu->neutral_values[cpu_neutral_flag_carry];
            }
            if (cache[0] == cache[1])
                cache[2] |= cpu->neutral_values[cpu_neutral_flag_zero];
            else
                cache[2] &= ~cpu->neutral_values[cpu_neutral_flag_zero];
            break;
        case cpu_opcode_inc:
            cpu_block_write(cpu, packet, cpu_block_read(cpu, packet, 0) + 1, 0);
            break;
        case cpu_opcode_dec:
            cpu_block_write(cpu, packet, cpu_block_read(cpu, packet, 0) - 1, 0);
            break;
        case cpu_opcode_push:
            for (uint8_t i = 0; i < packet->value_length; i++)
                cpu->push(cpu, cpu_block_read(cpu, packet, i));
            break;
        case cpu_opcode_pop:
            for (uint8_t i = 0; i < packet->value_length; i++)
                cpu_block_write(cpu, packet, cpu->pop(cpu), i);
            break;
        case cpu_opcode_mov:
            cpu_block_write(cpu, packet, cpu_block_read(cpu, packet, 1), 0);
            break;
        case cpu_opcode_lea:
            cpu_block_write(cpu, packet, cpu_block_address(cpu, packet, 1), 0);
            break;
        case cpu_opcode_xchg:
            cache[0] = cpu_block_read(cpu, packet, 0);
            cpu_block_write(cpu, packet, cpu_block_read(cpu, packet, 1), 0);
            cpu_block_write(cpu, packet, cache[0], 1);
            break;
        case cpu_opcode_jmp_far:
            cpu->pc_base = cpu_block_read(cpu, packet, 0);
            cpu->pc = cpu_block_read(cpu, packet, 1);
            goto end;
        case cpu_opcode_jcc_far:
            if (cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size) &
                cpu_block_read(cpu, packet, 2))
            {
                cpu->pc_base = cpu_block_read(cpu, packet, 0);
                cpu->pc = cpu_block_read(cpu, packet, 1);
                goto end;
            }
            break;
        case cpu_opcode_call_far:
            cpu->push(cpu, cpu->pc + packet->opcode_size);
            cpu->push(cpu, cpu->pc_base);
            cpu->pc_base = cpu_block_read(cpu, packet, 0);
            cpu->pc = cpu_block_read(cpu, packet, 1);
            goto end;
        case cpu_opcode_ret_far:
            cpu->pc = cpu->pop(cpu);
            cpu->pc_base = cpu->pop(cpu);
            goto end;
        case cpu_opcode_jmp_near:
            cpu->pc += cpu_block_sread(cpu, packet, 0);
            for (uint16_t j = 0; j < CPU_BLOCK_SIZE; j++)
            {
                cpu_block_t *packet = &cpu->code_block[j];
                if (packet->pc == cpu->pc)
                {
                    i = j;
                    okay = true;
                }
            }
            if (!okay)
                goto end;
            goto skip_pc_add;
        case cpu_opcode_jcc_near:
            if (cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size) &
                cpu_block_read(cpu, packet, 1))
            {
                cpu->pc += cpu_block_sread(cpu, packet, 0);
                for (uint16_t j = 0; j < CPU_BLOCK_SIZE; j++)
                {
                    cpu_block_t *packet = &cpu->code_block[j];
                    if (packet->pc == cpu->pc)
                    {
                        i = j;
                        okay = true;
                    }
                }
                if (!okay)
                    goto end;
                goto skip_pc_add;
            }
            break;
        case cpu_opcode_call_near:
            cpu->push(cpu, cpu->pc + packet->opcode_size);
            cpu->pc += cpu_block_sread(cpu, packet, 0);
            for (uint16_t j = 0; j < CPU_BLOCK_SIZE; j++)
            {
                cpu_block_t *packet = &cpu->code_block[j];
                if (packet->pc == cpu->pc)
                {
                    i = j;
                    okay = true;
                }
            }
            if (!okay)
                goto end;
            goto skip_pc_add;
        case cpu_opcode_ret_near:
            cpu->pc = cpu->pop(cpu);
            for (uint16_t j = 0; j < CPU_BLOCK_SIZE; j++)
            {
                cpu_block_t *packet = &cpu->code_block[j];
                if (packet->pc == cpu->pc)
                {
                    i = j;
                    okay = true;
                }
            }
            if (!okay)
                goto end;
            goto skip_pc_add;
        }
        cpu->pc += packet->opcode_size, i++;
    skip_pc_add:
        cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_instruction_pointer], cpu->pc, cpu->regs_size);
        continue;
    }
end:
    if (cpu)
        cpu->flags |= cpu_flag_stoped;
    return;
}