#include "cpu_x86.h"

uint16_t x86_jcc_flag[] = {
    x86_flags_OF,
    x86_flags_OF,
    x86_flags_CF,
    x86_flags_CF,
    x86_flags_ZF,
    x86_flags_ZF,
    x86_flags_CF | x86_flags_ZF,
    x86_flags_CF | x86_flags_ZF,
};

bool x86_jcc_map[] = {
    1,
    0,
    1,
    0,
    1,
    0,
    0,
    1,
};

uint16_t x86_rm_reg16_seg[] = {
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ss,
    x86_reg_ss,
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ss,
    x86_reg_ds,
};

uint16_t x86_rm_reg32_seg[] = {
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ss,
    x86_reg_ss,
    x86_reg_ds,
    x86_reg_ds,
};

uint16_t x86_rm_reg16_x[] = {
    x86_reg_rbx,
    x86_reg_rbx,
    x86_reg_rbp,
    x86_reg_rbp,
    x86_reg_rsi,
    x86_reg_rdi,
    x86_reg_rbp,
    x86_reg_rbx,
};

uint16_t x86_rm_reg16_y[] = {
    x86_reg_rsi,
    x86_reg_rdi,
    x86_reg_rsi,
    x86_reg_rdi,
    x86_reg_end,
    x86_reg_end,
    x86_reg_end,
    x86_reg_end,
};

uint64_t x86_size_max[] = {
    0x00,
    0xff,
    0xffff,
    0xffffff,
    0xffffffff,
    0xffffffffff,
    0xffffffffffff,
    0xffffffffffffff,
    0xffffffffffffffff,
};

uint64_t x86_size_half[] = {
    0x00 / 2 + 1,
    0xff / 2 + 1,
    0xffff / 2 + 1,
    0xffffff / 2 + 1,
    0xffffffff / 2 + 1,
    0xffffffffff / 2 + 1,
    0xffffffffffff / 2 + 1,
    0xffffffffffffff / 2 + 1,
    0xffffffffffffffff / 2 + 1,
};

uint8_t x86_rm_sizes[] = {
    2,
    2,
    2,
    4,
    4,
    8,
    8,
    8,
    8,
};

uint64_t x86_read_reg(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    if (reg > x86_reg_end - (size - 1))
        return 0;
    uint8_t *ptr = &cpu->regs[reg];
    switch (size)
    {
    case 0x01:
        return ptr[0];
    case 0x02:
        return ptr[0] |
               ((uint16_t)ptr[1] << 8);
    case 0x04:
        return ptr[0] |
               ((uint16_t)ptr[1] << 8) |
               ((uint32_t)ptr[2] << 16) |
               ((uint32_t)ptr[3] << 24);
    case 0x08:
        return ptr[0] |
               ((uint16_t)ptr[1] << 8) |
               ((uint32_t)ptr[2] << 16) |
               ((uint32_t)ptr[3] << 24) |
               ((uint64_t)ptr[4] << 32) |
               ((uint64_t)ptr[5] << 40) |
               ((uint64_t)ptr[6] << 48) |
               ((uint64_t)ptr[7] << 56);
    }
    return 0;
}

int64_t x86_sread_reg(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    if (reg > x86_reg_end - (size - 1))
        return 0;
    uint8_t *ptr = &cpu->regs[reg];
    switch (size)
    {
    case 0x01:
        return (int8_t)ptr[0];
    case 0x02:
        return (int16_t)(ptr[0] |
                         ((uint16_t)ptr[1] << 8));
    case 0x04:
        return (int32_t)(ptr[0] |
                         ((uint16_t)ptr[1] << 8) |
                         ((uint32_t)ptr[2] << 16) |
                         ((uint32_t)ptr[3] << 24));
    case 0x08:
        return (int64_t)(ptr[0] |
                         ((uint16_t)ptr[1] << 8) |
                         ((uint32_t)ptr[2] << 16) |
                         ((uint32_t)ptr[3] << 24) |
                         ((uint64_t)ptr[4] << 32) |
                         ((uint64_t)ptr[5] << 40) |
                         ((uint64_t)ptr[6] << 48) |
                         ((uint64_t)ptr[7] << 56));
    }
    return 0;
}

void x86_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value, uint8_t size)
{
    if (reg > x86_reg_end - (size - 1))
        return;
    uint8_t *ptr = &cpu->regs[reg];
    if (reg >= x86_reg_gs && reg <= x86_reg_ss)
    {
        *(uint64_t *)&cpu->cache[x86_cache_seg_gs + reg] = value << 4;
    }
    switch (size)
    {
    case 0x01:
        ptr[0] = value & 0xff;
        break;
    case 0x02:
        ptr[0] = value & 0xff;
        ptr[1] = (value >> 8) & 0xff;
        break;
    case 0x04:
        ptr[0] = value & 0xff;
        ptr[1] = (value >> 8) & 0xff;
        ptr[2] = (value >> 16) & 0xff;
        ptr[3] = (value >> 24) & 0xff;
        break;
    case 0x08:
        ptr[0] = value & 0xff;
        ptr[1] = (value >> 8) & 0xff;
        ptr[2] = (value >> 16) & 0xff;
        ptr[3] = (value >> 24) & 0xff;
        ptr[4] = (value >> 32) & 0xff;
        ptr[5] = (value >> 40) & 0xff;
        ptr[6] = (value >> 48) & 0xff;
        ptr[7] = (value >> 56) & 0xff;
        break;
    }
}

void x86_set_flags(cpu_t *cpu, uint64_t result, uint8_t size)
{
    uint64_t flags = x86_read_reg(cpu, x86_reg_rflags, 4);
    uint64_t max_size = x86_size_max[size], half_size = x86_size_half[size];
    if (result > max_size)
        flags |= x86_flags_CF;
    else
        flags &= ~x86_flags_CF;
    if (result > half_size)
        flags |= x86_flags_OF;
    else
        flags &= ~x86_flags_OF;
    if (~result & max_size)
        flags |= x86_flags_ZF;
    else
        flags &= ~x86_flags_ZF;
    if (result & half_size)
        flags |= x86_flags_SF;
    else
        flags &= ~x86_flags_SF;
    if (~(result & max_size) & 0x01)
        flags |= x86_flags_PF;
    else
        flags &= ~x86_flags_PF;
    x86_write_reg(cpu, x86_reg_rflags, flags, 4);
}

uint64_t x86_read_pc(cpu_t *cpu, uint8_t size)
{
    uint64_t ret = memory_read(*(uint64_t *)&cpu->cache[x86_cache_seg_cs] + cpu->pc, size, 0);
    cpu->pc += size;
    return ret;
}

int64_t x86_sread_pc(cpu_t *cpu, uint8_t size)
{
    int64_t ret = memory_sread(*(uint64_t *)&cpu->cache[x86_cache_seg_cs] + cpu->pc, size, 0);
    cpu->pc += size;
    return ret;
}

uint8_t x86_get_operand_size(cpu_t *cpu)
{
    if (cpu->override & x86_override_dword_operand)
        return 4;
    return 2;
}

uint64_t x86_get_segment_address(cpu_t *cpu)
{
    if (cpu->override & x86_reg_gs)
        return *(uint64_t *)&cpu->cache[x86_cache_seg_gs];
    if (cpu->override & x86_reg_fs)
        return *(uint64_t *)&cpu->cache[x86_cache_seg_fs];
    if (cpu->override & x86_reg_es)
        return *(uint64_t *)&cpu->cache[x86_cache_seg_es];
    if (cpu->override & x86_reg_ds)
        return *(uint64_t *)&cpu->cache[x86_cache_seg_ds];
    if (cpu->override & x86_reg_cs)
        return *(uint64_t *)&cpu->cache[x86_cache_seg_cs];
    if (cpu->override & x86_reg_ss)
        return *(uint64_t *)&cpu->cache[x86_cache_seg_ss];
    return *(uint64_t *)&cpu->cache[x86_cache_seg_ds];
}

uint64_t x86_imm_address(cpu_t *cpu, uint8_t size)
{
    if (cpu->override & x86_override_dword_address)
        size = 4;
    return x86_get_segment_address(cpu) + x86_read_pc(cpu, size);
}

uint64_t x86_decode_modrm(cpu_t *cpu, x86_rm_t modrm, uint8_t size)
{
    x86_sib_t sib;
    switch (size)
    {
    case 0x01 ... 0x02:
        return CPU_BLOCK_MREG(x86_rm_reg16_x[modrm.rm],
                              x86_rm_reg16_y[modrm.rm],
                              1, 2);
    case 0x04:
        if (modrm.rm == 0x04)
        {
            sib.value = x86_read_pc(cpu, 1);
            if (sib.base != sib.index)
                return CPU_BLOCK_MREG(x86_regs[sib.base],
                                      x86_regs[sib.index],
                                      (1 << sib.scale), 4);
            return CPU_BLOCK_MREG(x86_regs[sib.base],
                                  x86_regs[sib.index],
                                  0, 4);
        }
        return CPU_BLOCK_MREG(x86_regs[modrm.rm],
                              x86_regs[modrm.rm],
                              0, 4);
    }
    return 0;
}

void x86_decode(cpu_t *cpu, x86_rm_t modrm, bool check_operand, uint8_t size)
{
    cpu->cache[x86_cache_type_address] = cpu_type_mreg;
    switch (modrm.mod)
    {
    case 0x00:
        if (cpu->override & x86_override_dword_address)
            size = 4;
        *(uint64_t *)&cpu->cache[x86_cache_address0] = x86_decode_modrm(cpu, modrm, size);
        if (modrm.mod == 0x06)
        {
            cpu->cache[x86_cache_type_address] = cpu_type_mem;
            *(uint64_t *)&cpu->cache[x86_cache_address0] = x86_read_pc(cpu, x86_rm_sizes[size]);
            return;
        }
        return;
    case 0x01:
        if (cpu->override & x86_override_dword_address)
            size = 4;
        *(uint64_t *)&cpu->cache[x86_cache_address0] = x86_decode_modrm(cpu, modrm, size);
        *(int64_t *)&cpu->cache[x86_cache_address1] = x86_sread_pc(cpu, 1);
        return;
    case 0x02:
        if (cpu->override & x86_override_dword_address)
            size = 4;
        *(uint64_t *)&cpu->cache[x86_cache_address0] = x86_decode_modrm(cpu, modrm, size);
        *(int64_t *)&cpu->cache[x86_cache_address1] = x86_read_pc(cpu, x86_rm_sizes[size]);
        return;
    case 0x03:
        if (check_operand && size > 1)
            size = x86_get_operand_size(cpu);
        cpu->cache[x86_cache_type_address] = cpu_type_reg;
        *(uint64_t *)&cpu->cache[x86_cache_address0] = x86_mod3_reg[size][modrm.rm];
        return;
    }
}

uint64_t x86_rm_read(cpu_t *cpu, x86_rm_t modrm, uint8_t size)
{
    if (modrm.mod < 0x03)
        return memory_read(*(uint64_t *)&cpu->cache[x86_cache_address0],
                           size,
                           0);
    else
        return x86_read_reg(cpu, *(uint16_t *)&cpu->cache[x86_cache_address0], size);
}

int64_t x86_rm_sread(cpu_t *cpu, x86_rm_t modrm, uint8_t size)
{
    if (modrm.mod < 0x03)
        return memory_sread(*(uint64_t *)&cpu->cache[x86_cache_address0],
                            size,
                            0);
    else
        return x86_sread_reg(cpu, *(uint16_t *)&cpu->cache[x86_cache_address0], size);
}

uint64_t x86_rm_read_effective_address(cpu_t *cpu, x86_rm_t modrm, uint8_t size)
{
    if (modrm.mod < 0x03)
        return *(uint64_t *)&cpu->cache[x86_cache_address0];
    else
        return x86_read_reg(cpu, *(uint16_t *)&cpu->cache[x86_cache_address0], size);
}

void x86_rm_write(cpu_t *cpu, x86_rm_t modrm, uint64_t value, uint8_t size)
{
    if (modrm.mod < 0x03)
        return memory_write(*(uint64_t *)&cpu->cache[x86_cache_address0],
                            value,
                            size,
                            0);
    else
        return x86_write_reg(cpu, *(uint16_t *)&cpu->cache[x86_cache_address0], value, size);
}

void x86_reg_write(cpu_t *cpu, const uint16_t *regs, x86_rm_t modrm, uint64_t value, uint8_t size)
{
    return x86_write_reg(cpu, regs[modrm.reg], value, size);
}

void x86_push(cpu_t *cpu, uint64_t value)
{
    uint8_t size = cpu->cache[x86_cache_size];
    uint64_t stack = x86_read_reg(cpu, x86_reg_rsp, 4) - size;
    x86_write_reg(cpu, x86_reg_rsp, stack, 4);
    memory_write(stack, value, size, 0);
}

uint64_t x86_pop(cpu_t *cpu)
{
    uint8_t size = cpu->cache[x86_cache_size];
    uint64_t stack = x86_read_reg(cpu, x86_reg_rsp, 4);
    x86_write_reg(cpu, x86_reg_rsp, stack + size, 4);
    return memory_read(stack, size, 0);
}

bool x86_get_flag(cpu_t *cpu, uint64_t flag)
{
    return x86_read_reg(cpu, x86_reg_rflags, 4) & flag ? 1 : 0;
}

void x86_cmp(cpu_t *cpu, uint64_t value1, uint64_t value2)
{
    uint64_t flags = x86_read_reg(cpu, x86_reg_rflags, 4);
    if (value1 < value2)
        flags |= x86_flags_CF;
    else
        flags &= ~x86_flags_CF;
    if (value1 == value2)
        flags |= x86_flags_ZF;
    else
        flags &= ~x86_flags_ZF;
    x86_write_reg(cpu, x86_reg_rflags, flags, 4);
}

void x86_opcode_c6_c7(cpu_t *cpu, uint8_t opcode)
{
    uint8_t operand_size = x86_get_operand_size(cpu);
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    if (opcode & 0x01)
    {
        x86_decode(cpu, modrm, 1, 2);
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                      *(uint64_t *)&cpu->cache[x86_cache_address0],
                                      *(int64_t *)&cpu->cache[x86_cache_address1]),
                      CPU_BLOCK_VALUE(cpu_type_int, operand_size,
                                      x86_read_pc(cpu, operand_size),
                                      0));
    }
    else
    {
        x86_decode(cpu, modrm, 1, 1);
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                      *(uint64_t *)&cpu->cache[x86_cache_address0],
                                      *(int64_t *)&cpu->cache[x86_cache_address1]),
                      CPU_BLOCK_VALUE(cpu_type_int, 1,
                                      x86_read_pc(cpu, 1),
                                      0));
    }
}

void x86_opcode_8e(cpu_t *cpu, uint8_t opcode)
{
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                  CPU_BLOCK_VALUE(cpu_type_reg, 2, x86_sregs[modrm.reg], 0),
                  CPU_BLOCK_VALUE(cpu_type_reg, 2, x86_regs[modrm.rm], 0));
}

void x86_opcode_8d(cpu_t *cpu, uint8_t opcode)
{
    uint8_t operand_size = x86_get_operand_size(cpu);
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    x86_decode(cpu, modrm, 1, 2);
    cpu_block_add(cpu, cpu_opcode_lea, 0, 2,
                  CPU_BLOCK_VALUE(cpu_type_reg, operand_size,
                                  x86_regs[modrm.reg],
                                  0),
                  CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                  *(uint64_t *)&cpu->cache[x86_cache_address0],
                                  *(int64_t *)&cpu->cache[x86_cache_address1]));
}

void x86_opcode_8c(cpu_t *cpu, uint8_t opcode)
{
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                  CPU_BLOCK_VALUE(cpu_type_reg, 2, x86_regs[modrm.rm], 0),
                  CPU_BLOCK_VALUE(cpu_type_reg, 2, x86_sregs[modrm.reg], 0));
}

void x86_opcode_8a_8b(cpu_t *cpu, uint8_t opcode)
{
    uint8_t operand_size = x86_get_operand_size(cpu);
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    if (opcode & 0x01)
    {
        x86_decode(cpu, modrm, 1, 2);
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size,
                                      x86_regs[modrm.reg],
                                      0),
                      CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                      *(uint64_t *)&cpu->cache[x86_cache_address0],
                                      *(int64_t *)&cpu->cache[x86_cache_address1]));
    }
    else
    {
        x86_decode(cpu, modrm, 1, 1);
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu_type_reg, 1,
                                      x86_regs8[modrm.reg],
                                      0),
                      CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], 1,
                                      *(uint64_t *)&cpu->cache[x86_cache_address0],
                                      *(int64_t *)&cpu->cache[x86_cache_address1]));
    }
}

void x86_opcode_88_89(cpu_t *cpu, uint8_t opcode)
{
    uint8_t operand_size = x86_get_operand_size(cpu);
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    if (opcode & 0x01)
    {
        x86_decode(cpu, modrm, 1, 2);
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                      *(uint64_t *)&cpu->cache[x86_cache_address0],
                                      *(int64_t *)&cpu->cache[x86_cache_address1]),
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size,
                                      x86_regs[modrm.reg],
                                      0));
    }
    else
    {
        x86_decode(cpu, modrm, 1, 1);
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], 1,
                                      *(uint64_t *)&cpu->cache[x86_cache_address0],
                                      *(int64_t *)&cpu->cache[x86_cache_address1]),
                      CPU_BLOCK_VALUE(cpu_type_reg, 1,
                                      x86_regs8[modrm.reg],
                                      0));
    }
}

/*void x86_opcode_80_83(cpu_t *cpu, uint8_t opcode)
{
    uint8_t operand_size = x86_get_operand_size(cpu);
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    uint8_t size1 = 0;
    uint8_t size2 = 0;
    switch (opcode & 0x03)
    {
    case 0x00:
        size1 = 1, size2 = 1;
        break;
    case 0x02:
        size1 = operand_size, size2 = operand_size;
        break;
    case 0x03:
        size1 = operand_size, size2 = 1;
        break;
    }
    if (!size1 || !size2)
        return;
    x86_decode(cpu, modrm, 1, size1);
    int64_t simm = x86_sread_pc(cpu, size2);
    uint64_t result;
    switch (modrm.reg)
    {
    case 0x00:
        result = x86_rm_read(cpu, modrm, size1) + simm;
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x01:
        result = x86_rm_read(cpu, modrm, size1) | simm;
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x02:
        result = x86_rm_read(cpu, modrm, size1) + simm + x86_get_flag(cpu, x86_flags_CF);
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x03:
        result = x86_rm_read(cpu, modrm, size1) - simm + x86_get_flag(cpu, x86_flags_CF);
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x04:
        result = x86_rm_read(cpu, modrm, size1) & simm;
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x05:
        result = x86_rm_read(cpu, modrm, size1) - simm;
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x06:
        result = x86_rm_read(cpu, modrm, size1) ^ simm;
        x86_set_flags(cpu, result, size1);
        x86_rm_write(cpu, modrm, result, size1);
        break;
    case 0x07:
        x86_cmp(cpu, x86_rm_read(cpu, modrm, size1), simm);
        break;
    }
}*/

void x86_opcode_0f(cpu_t *cpu, uint8_t opcode)
{
    uint8_t operand_size = x86_get_operand_size(cpu);
    int64_t scache0;
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    switch (modrm.reg)
    {
    case 0x00:
        switch (modrm.mod)
        {
        case 0x00:
            modrm.value = x86_read_pc(cpu, 1);
            switch (modrm.reg)
            {
            case 0x02:
                x86_decode(cpu, modrm, 1, 2);
                cpu_block_add(cpu, cpu_opcode_lea, 0, 2,
                              CPU_BLOCK_VALUE(cpu_type_reg, 8, x86_reg_gdtr, 0),
                              CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                              *(uint64_t *)&cpu->cache[x86_cache_address0],
                                              *(int64_t *)&cpu->cache[x86_cache_address1]));
                break;
            case 0x03:
                x86_decode(cpu, modrm, 1, 2);
                cpu_block_add(cpu, cpu_opcode_lea, 0, 2,
                              CPU_BLOCK_VALUE(cpu_type_reg, 8, x86_reg_idtr, 0),
                              CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], operand_size,
                                              *(uint64_t *)&cpu->cache[x86_cache_address0],
                                              *(int64_t *)&cpu->cache[x86_cache_address1]));
                break;
            }
            break;
        case 0x02:
            scache0 = x86_sread_pc(cpu, cpu->cache[x86_cache_size]);
            if (x86_jcc_map[modrm.rm])
            {
                cpu_block_add(cpu, cpu_opcode_jcc_near, 1, 2,
                              CPU_BLOCK_VALUE(cpu_type_int, cpu->cache[x86_cache_size], scache0, 0),
                              CPU_BLOCK_VALUE(cpu_type_int, cpu->cache[x86_cache_size],
                                              x86_jcc_flag[modrm.rm], 0));
            }
            else
            {
                cpu_block_add(cpu, cpu_opcode_jncc_near, 1, 2,
                              CPU_BLOCK_VALUE(cpu_type_int, cpu->cache[x86_cache_size], scache0, 0),
                              CPU_BLOCK_VALUE(cpu_type_int, cpu->cache[x86_cache_size],
                                              x86_jcc_flag[modrm.rm], 0));
            }
            break;
        }
        break;
    case 0x04:
        switch (modrm.mod)
        {
        case 0x00:
            switch (modrm.rm)
            {
            case 0x00:
                modrm.value = x86_read_pc(cpu, 1);
                cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                              CPU_BLOCK_VALUE(cpu_type_reg, 4, x86_regs[modrm.rm], 0),
                              CPU_BLOCK_VALUE(cpu_type_reg, 4, x86_regscontrol[modrm.reg], 0));
                break;
            case 0x02:
                modrm.value = x86_read_pc(cpu, 1);
                cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                              CPU_BLOCK_VALUE(cpu_type_reg, 4, x86_regscontrol[modrm.reg], 0),
                              CPU_BLOCK_VALUE(cpu_type_reg, 4, x86_regs[modrm.rm], 0));
                break;
            }
            break;
        }
        break;
    case 0x05:
        modrm.value = x86_read_pc(cpu, 1);
        x86_decode(cpu, modrm, 1, 2);
        x86_reg_write(cpu,
                      x86_mod3_reg[operand_size],
                      modrm,
                      x86_read_reg(cpu, x86_mod3_reg[operand_size][modrm.reg], operand_size) *
                          x86_rm_sread(cpu, modrm, operand_size),
                      operand_size);
        break;
    case 0x06:
        switch (modrm.mod)
        {
        case 0x02:
            switch (modrm.rm)
            {
            case 0x06:
                modrm.value = x86_read_pc(cpu, 1);
                x86_decode(cpu, modrm, 0, 1);
                cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                              CPU_BLOCK_VALUE(cpu_type_reg, operand_size,
                                              x86_regs[modrm.reg],
                                              0),
                              CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], 1,
                                              *(uint64_t *)&cpu->cache[x86_cache_address0],
                                              *(int64_t *)&cpu->cache[x86_cache_address1]));
                break;
            case 0x07:
                modrm.value = x86_read_pc(cpu, 1);
                x86_decode(cpu, modrm, 0, 2);
                cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                              CPU_BLOCK_VALUE(cpu_type_reg, operand_size,
                                              x86_regs[modrm.reg],
                                              0),
                              CPU_BLOCK_VALUE(cpu->cache[x86_cache_type_address], 2,
                                              *(uint64_t *)&cpu->cache[x86_cache_address0],
                                              *(int64_t *)&cpu->cache[x86_cache_address1]));
                break;
            }
            break;
        }
        break;
    }
}

void x86_opcode_unsigned_operation_00_01(cpu_t *cpu, uint8_t opcode, uint8_t type)
{
    x86_rm_t modrm = {.value = x86_read_pc(cpu, 1)};
    modrm.value |= 0xff;
}

uint8_t x86_recompile(cpu_t *cpu)
{
    uint8_t operand_size;
    uint64_t x86_cache[4];
    uint8_t ret = 0, opcode = 0, bit_check = 0;
    uint16_t old_override = cpu->override;
    if (x86_read_reg(cpu, x86_reg_cr0, 4) & x86_cr0_PE)
    {
        cpu->cache[x86_cache_size] = 4;
        old_override |= (1 << 0) | (1 << 1);
        cpu->override = old_override;
    }
    else
    {
        cpu->cache[x86_cache_size] = 2;
        old_override &= ~((1 << 0) | (1 << 1));
        cpu->override = old_override;
    }
start:
    operand_size = x86_get_operand_size(cpu);
    opcode = x86_read_pc(cpu, 1);
    bit_check = 1 << (opcode & 0x01);
    switch (opcode)
    {
    case 0x0f:
        x86_opcode_0f(cpu, opcode);
        break;
    case 0x30 ... 0x31:
        x86_opcode_unsigned_operation_00_01(cpu, opcode, x86_operation_xor);
        break;
    case 0x40 ... 0x47:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_inc, 0, 1,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_regs[opcode], 0));
        break;
    case 0x48 ... 0x4f:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_dec, 0, 1,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_regs[opcode], 0));
        break;
    case 0x50 ... 0x57:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_push, 0, 1,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_regs[opcode], 0));
        break;
    case 0x58 ... 0x5f:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_pop, 0, 1,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_regs[opcode], 0));
        break;
    case 0x66 ... 0x67:
        if (cpu->override & bit_check)
            cpu->override &= ~bit_check;
        else
            cpu->override |= bit_check;
        goto start;
    /*case 0x80 ... 0x83:
        x86_opcode_80_83(cpu, opcode);
        break;*/
    case 0x88 ... 0x89:
        x86_opcode_88_89(cpu, opcode);
        break;
    case 0x8a ... 0x8b:
        x86_opcode_8a_8b(cpu, opcode);
        break;
    case 0x8c:
        x86_opcode_8c(cpu, opcode);
        break;
    case 0x8d:
        x86_opcode_8d(cpu, opcode);
        break;
    case 0x8e:
        x86_opcode_8e(cpu, opcode);
        break;
    case 0x90 ... 0x97:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_xchg, 0, 2,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_regs[opcode], 0),
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_reg_rax, 0));
        break;
    /*case 0xa0 ... 0xa1:
        if (opcode & 0x01)
        {
            x86_cache[0] = x86_imm_address(cpu, x86_cache[0]);
            x86_write_reg(cpu, x86_reg_rax, memory_read(x86_cache[0], operand_size, 0), operand_size);
        }
        else
        {
            x86_write_reg(cpu, x86_reg_rax, memory_read(x86_imm_address(cpu, 1), 1, 0), 1);
        }
        break;*/
    case 0xb0 ... 0xb7:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu_type_reg, 1, x86_regs8[opcode], 0),
                      CPU_BLOCK_VALUE(cpu_type_int, 1, x86_read_pc(cpu, 1), 0));
        break;
    case 0xb8 ... 0xbf:
        opcode &= 0x07;
        cpu_block_add(cpu, cpu_opcode_mov, 0, 2,
                      CPU_BLOCK_VALUE(cpu_type_reg, operand_size, x86_regs[opcode], 0),
                      CPU_BLOCK_VALUE(cpu_type_int, operand_size, x86_read_pc(cpu, operand_size), 0));
        break;
    case 0xc3:
        cpu_block_add(cpu, cpu_opcode_ret_near, 0, 0);
        break;
    case 0xc6 ... 0xc7:
        x86_opcode_c6_c7(cpu, opcode);
        break;
    case 0xe8 ... 0xe9:
        if (opcode & 0x01)
        {
            cpu_block_add(cpu, cpu_opcode_jmp_near, 1, 1,
                          CPU_BLOCK_VALUE(cpu_type_int, operand_size, x86_sread_pc(cpu, operand_size), 0));
        }
        else
        {
            cpu_block_add(cpu, cpu_opcode_call_near, 1, 1,
                          CPU_BLOCK_VALUE(cpu_type_int, operand_size, x86_sread_pc(cpu, operand_size), 0));
        }
        break;
    case 0xea ... 0xeb:
        if (opcode & 0x01)
        {
            cpu_block_add(cpu, cpu_opcode_jmp_near, 1, 1,
                          CPU_BLOCK_VALUE(cpu_type_int, 1, x86_sread_pc(cpu, 1), 0));
        }
        else
        {
            x86_cache[0] = x86_read_pc(cpu, operand_size);
            x86_cache[1] = x86_read_pc(cpu, 2);
            cpu_block_add(cpu, cpu_opcode_jmp_far, 0, 2,
                          CPU_BLOCK_VALUE(cpu_type_int, 2, x86_cache[1], 0),
                          CPU_BLOCK_VALUE(cpu_type_int, operand_size, x86_cache[0], 0));
        }
        break;
    /*case 0xec:
        x86_write_reg(cpu, x86_reg_rax, io_read(x86_read_reg(cpu, x86_reg_rdx, 2), 1), 1);
        break;
    case 0xed:
        x86_write_reg(cpu, x86_reg_rax,
                      io_read(x86_read_reg(cpu, x86_reg_rdx, 2), operand_size),
                      operand_size);
        break;
    case 0xee:
        io_write(x86_read_reg(cpu, x86_reg_rdx, 2), x86_read_reg(cpu, x86_reg_rax, 1), 1);
        break;
    case 0xef:
        io_write(x86_read_reg(cpu, x86_reg_rdx, 2),
                 x86_read_reg(cpu, x86_reg_rax, operand_size),
                 operand_size);
        break;
    case 0xfa ... 0xfb:
        if (opcode & 0x01)
            cpu_write_reg(cpu, x86_reg_rflags, x86_read_reg(cpu, x86_reg_rflags, 4) | x86_flags_IF, 4);
        else
            cpu_write_reg(cpu, x86_reg_rflags, x86_read_reg(cpu, x86_reg_rflags, 4) & ~x86_flags_IF, 4);
        break;
    case 0xfc ... 0xfd:
        if (opcode & 0x01)
            cpu_write_reg(cpu, x86_reg_rflags, x86_read_reg(cpu, x86_reg_rflags, 4) | x86_flags_DF, 4);
        else
            cpu_write_reg(cpu, x86_reg_rflags, x86_read_reg(cpu, x86_reg_rflags, 4) & ~x86_flags_DF, 4);
        break;*/
    default:
        cpu->pc--;
        ret++;
        break;
    }
    cpu->override = old_override;
    x86_write_reg(cpu, x86_reg_rip, cpu->pc, 4);
    return ret;
}

void x86_reset(cpu_t *cpu)
{
    memset(cpu->regs, 0, x86_reg_end);
    memset(cpu->cache, 0, x86_cache_end);
    x86_write_reg(cpu, x86_reg_rax, rand(), cpu->regs_size);
    x86_write_reg(cpu, x86_reg_gs, 0x0000, 2);
    x86_write_reg(cpu, x86_reg_fs, 0x0000, 2);
    x86_write_reg(cpu, x86_reg_es, 0x0000, 2);
    x86_write_reg(cpu, x86_reg_ds, 0x0000, 2);
    x86_write_reg(cpu, x86_reg_cs, 0xffff, 2);
    x86_write_reg(cpu, x86_reg_ss, 0x0000, 2);
    cpu->pc = 0;
}

cpu_t *x86_setup()
{
    cpu_t *ret = malloc(sizeof(cpu_t));
    if (!ret)
        return (cpu_t *)NULL;
    memset(ret, 0, sizeof(cpu_t));
    ret->neutral_values[cpu_neutral_reg_instruction_pointer] = x86_reg_rip;
    ret->neutral_values[cpu_neutral_reg_code_segment] = x86_reg_cs;
    ret->neutral_values[cpu_neutral_reg_flags] = x86_reg_rflags;
    ret->neutral_values[cpu_neutral_reg_stack] = x86_reg_rsp;
    ret->regs_size = 8;
    ret->regs = malloc(x86_reg_end);
    ret->cache = malloc(x86_cache_end);
    if (!ret->regs || !ret->cache)
    {
        free(ret->regs);
        free(ret->cache);
        free(ret);
        return (cpu_t *)NULL;
    }
    ret->push = x86_push;
    ret->pop = x86_pop;
    ret->read_reg = x86_read_reg;
    ret->sread_reg = x86_sread_reg;
    ret->write_reg = x86_write_reg;
    ret->reset = x86_reset;
    ret->recompile = x86_recompile;
    cpu_reset(ret);
    return ret;
}
