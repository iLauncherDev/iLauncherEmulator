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
    if (cpu->recompile)
    {
        uint64_t pc = cpu->pc;
        for (cpu->code_block_index = 0;
             cpu->code_block_index < CPU_BLOCK_SIZE;
             cpu->code_block_index++)
        {
            uint16_t index = cpu->code_block_index;
            cpu->code_block[index].pc = cpu->pc;
            cpu->recompile(cpu);
            cpu->code_block[index].opcode_size = cpu->pc - cpu->code_block[index].pc;
        }
        cpu->pc = pc;
    }
    cpu->flags &= ~cpu_flag_stoped;
end:
    return;
}

void cpu_block_add(cpu_t *cpu, uint8_t opcode, uint8_t sign, uint8_t value_length, ...)
{
    va_list args;
    uint16_t index;
    if (!cpu)
        goto end;
    va_start(args, value_length);
    index = cpu->code_block_index;
    cpu->code_block[index].opcode = opcode;
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

uint64_t cpu_block_address(cpu_t *cpu, cpu_block_t *block, uint8_t index)
{
    uint8_t size;
    switch (block->values[index].type)
    {
    case cpu_type_int:
        return block->values[index].value;
    case cpu_type_reg:
        return cpu_read_reg(cpu, block->values[index].value, block->values[index].size);
    case cpu_type_mem:
        return block->values[index].value;
    case cpu_type_mreg:
        size = (block->values[index].value >> 56) & 0xff;
        return cpu_read_reg(cpu,
                            block->values[index].value & 0xffff,
                            size) +
               cpu_read_reg(cpu,
                            (block->values[index].value >> 16) & 0xffff,
                            size) *
                   ((block->values[index].value >> 32) & 0xffff) +
               block->values[index].offset;
    }
    return 0;
}

uint64_t cpu_block_read(cpu_t *cpu, cpu_block_t *block, uint8_t index)
{
    uint8_t size;
    switch (block->values[index].type)
    {
    case cpu_type_int:
        return block->values[index].value;
    case cpu_type_reg:
        return cpu_read_reg(cpu, block->values[index].value, block->values[index].size);
    case cpu_type_mem:
        return memory_read(block->values[index].value, block->values[index].size, cpu->big_endian);
    case cpu_type_mreg:
        size = (block->values[index].value >> 56) & 0xff;
        return memory_read(cpu_read_reg(cpu,
                                        block->values[index].value & 0xffff,
                                        size) +
                               cpu_read_reg(cpu,
                                            (block->values[index].value >> 16) & 0xffff,
                                            size) *
                                   ((block->values[index].value >> 32) & 0xffff) +
                               block->values[index].offset,
                           block->values[index].size, cpu->big_endian);
    }
    return 0;
}

int64_t cpu_block_sread(cpu_t *cpu, cpu_block_t *block, uint8_t index)
{
    uint8_t size;
    switch (block->values[index].type)
    {
    case cpu_type_int:
        return (int64_t)block->values[index].value;
    case cpu_type_reg:
        return cpu_sread_reg(cpu, block->values[index].value, block->values[index].size);
    case cpu_type_mem:
        return memory_sread(block->values[index].value, block->values[index].size, cpu->big_endian);
    case cpu_type_mreg:
        size = (block->values[index].value >> 56) & 0xff;
        return memory_sread(cpu_read_reg(cpu,
                                         block->values[index].value & 0xffff,
                                         size) +
                                cpu_read_reg(cpu,
                                             (block->values[index].value >> 16) & 0xffff,
                                             size) *
                                    ((block->values[index].value >> 32) & 0xffff) +
                                block->values[index].offset,
                            block->values[index].size, cpu->big_endian);
    }
    return 0;
}

void cpu_block_write(cpu_t *cpu, cpu_block_t *block, uint64_t value, uint8_t index)
{
    uint8_t size;
    switch (block->values[index].type)
    {
    case cpu_type_reg:
        cpu_write_reg(cpu, block->values[index].value, value, block->values[index].size);
        return;
    case cpu_type_mem:
        memory_write(block->values[index].value, value, block->values[index].size, cpu->big_endian);
        return;
    case cpu_type_mreg:
        size = (block->values[index].value >> 56) & 0xff;
        memory_write(cpu_read_reg(cpu,
                                  block->values[index].value & 0xffff,
                                  size) +
                         cpu_read_reg(cpu,
                                      (block->values[index].value >> 16) & 0xffff,
                                      size) *
                             ((block->values[index].value >> 32) & 0xffff) +
                         block->values[index].offset,
                     value,
                     block->values[index].size, cpu->big_endian);
        return;
    }
}

void cpu_execute(cpu_t *cpu)
{
    if (!cpu || cpu->flags & cpu_flag_stoped)
        goto end;
    uint64_t cache[4];
    cpu_block_t *block = cpu->code_block, *block_max = &cpu->code_block[CPU_BLOCK_SIZE];
    do
    {
        if (cpu->flags & cpu_flag_reset)
        {
            if (cpu->reset)
                cpu->reset(cpu);
            cpu->flags &= ~cpu_flag_reset;
            goto end;
        }
        cpu->pc += block->opcode_size;
        switch (block->opcode)
        {
        case cpu_opcode_exit:
            goto end;
        case cpu_opcode_cmp:
            cache[2] = cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size);
            if (block->sign)
            {
                cache[0] = cpu_block_sread(cpu, block, 0);
                cache[1] = cpu_block_sread(cpu, block, 1);
                if ((int64_t)cache[0] < (int64_t)cache[1])
                    cache[2] |= cpu->neutral_values[cpu_neutral_flag_carry];
                else
                    cache[2] &= ~cpu->neutral_values[cpu_neutral_flag_carry];
            }
            else
            {
                cache[0] = cpu_block_read(cpu, block, 0);
                cache[1] = cpu_block_read(cpu, block, 1);
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
            for (uint8_t i = 0; i < block->value_length; i++)
                cpu_block_write(cpu, block, cpu_block_read(cpu, block, i) + 1, i);
            break;
        case cpu_opcode_dec:
            for (uint8_t i = 0; i < block->value_length; i++)
                cpu_block_write(cpu, block, cpu_block_read(cpu, block, i) - 1, i);
            break;
        case cpu_opcode_push:
            for (uint8_t i = 0; i < block->value_length; i++)
                cpu->push(cpu, cpu_block_read(cpu, block, i));
            break;
        case cpu_opcode_pop:
            for (uint8_t i = 0; i < block->value_length; i++)
                cpu_block_write(cpu, block, cpu->pop(cpu), i);
            break;
        case cpu_opcode_mov:
            cpu_block_write(cpu, block, cpu_block_read(cpu, block, 1), 0);
            break;
        case cpu_opcode_lea:
            cpu_block_write(cpu, block, cpu_block_address(cpu, block, 1), 0);
            break;
        case cpu_opcode_xchg:
            cache[0] = cpu_block_read(cpu, block, 0);
            cpu_block_write(cpu, block, cpu_block_read(cpu, block, 1), 0);
            cpu_block_write(cpu, block, cache[0], 1);
            break;
        case cpu_opcode_jmp_far:
            cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_code_segment],
                          cpu_block_read(cpu, block, 0),
                          cpu->regs_size);
            cpu->pc = cpu_block_read(cpu, block, 1);
            goto end;
        case cpu_opcode_jcc_far:
            if (cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size) &
                cpu_block_read(cpu, block, 2))
            {
                cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_code_segment],
                              cpu_block_read(cpu, block, 0),
                              cpu->regs_size);
                cpu->pc = cpu_block_read(cpu, block, 1);
                goto end;
            }
            break;
        case cpu_opcode_jncc_far:
            if (~cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size) &
                cpu_block_read(cpu, block, 2))
            {
                cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_code_segment],
                              cpu_block_read(cpu, block, 0),
                              cpu->regs_size);
                cpu->pc = cpu_block_read(cpu, block, 1);
                goto end;
            }
            break;
        case cpu_opcode_call_far:
            cpu->push(cpu, cpu->pc + block->opcode_size);
            cpu->push(cpu, cpu_read_reg(cpu,
                                        cpu->neutral_values[cpu_neutral_reg_code_segment],
                                        cpu->regs_size));
            cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_code_segment],
                          cpu_block_address(cpu, block, 0),
                          cpu->regs_size);
            cpu->pc = cpu_block_address(cpu, block, 1);
            goto end;
        case cpu_opcode_ret_far:
            cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_code_segment],
                          cpu->pop(cpu),
                          cpu->regs_size);
            cpu->pc = cpu->pop(cpu);
            goto end;
        case cpu_opcode_jmp_near:
            cpu->pc += cpu_block_sread(cpu, block, 0);
            block = cpu->code_block;
            do
            {
                if (block->pc == cpu->pc)
                    goto loop_end;
                block++;
            } while (block < block_max);
            goto end;
        case cpu_opcode_jcc_near:
            if (cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size) &
                cpu_block_read(cpu, block, 1))
            {
                cpu->pc += cpu_block_sread(cpu, block, 0);
                block = cpu->code_block;
                do
                {
                    if (block->pc == cpu->pc)
                        goto loop_end;
                    block++;
                } while (block < block_max);
                goto end;
            }
            break;
        case cpu_opcode_jncc_near:
            if (~cpu_read_reg(cpu, cpu->neutral_values[cpu_neutral_reg_flags], cpu->regs_size) &
                cpu_block_read(cpu, block, 1))
            {
                cpu->pc += cpu_block_sread(cpu, block, 0);
                block = cpu->code_block;
                do
                {
                    if (block->pc == cpu->pc)
                        goto loop_end;
                    block++;
                } while (block < block_max);
                goto end;
            }
            break;
        case cpu_opcode_call_near:
            cpu->push(cpu, cpu->pc);
            cpu->pc += cpu_block_sread(cpu, block, 0);
            block = cpu->code_block;
            do
            {
                if (block->pc == cpu->pc)
                    goto loop_end;
                block++;
            } while (block < block_max);
            goto end;
        case cpu_opcode_ret_near:
            cpu->pc = cpu->pop(cpu);
            block = cpu->code_block;
            do
            {
                if (block->pc == cpu->pc)
                    goto loop_end;
                block++;
            } while (block < block_max);
            goto end;
        }
        cpu_write_reg(cpu, cpu->neutral_values[cpu_neutral_reg_instruction_pointer], cpu->pc, cpu->regs_size);
        block++;
    loop_end:
        continue;
    } while (block < block_max);
end:
    if (cpu)
        cpu->flags |= cpu_flag_stoped;
    return;
}