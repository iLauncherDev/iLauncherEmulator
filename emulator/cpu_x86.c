#include "cpu_x86.h"

uint8_t x86_rm_segment16[] = {
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ss,
    x86_reg_ss,
    x86_reg_ds,
    x86_reg_ds,
    x86_reg_ss,
    x86_reg_ds,
};

uint8_t x86_rm_reg32_x[] = {
    x86_reg_eax,
    x86_reg_ecx,
    x86_reg_edx,
    x86_reg_ebx,
    x86_reg_end,
    x86_reg_ebp,
    x86_reg_esi,
    x86_reg_edi,
};

uint8_t x86_rm_reg16_x[] = {
    x86_reg_bx,
    x86_reg_bx,
    x86_reg_bp,
    x86_reg_bp,
    x86_reg_si,
    x86_reg_di,
    x86_reg_bp,
    x86_reg_bx,
};

uint8_t x86_rm_reg16_y[] = {
    x86_reg_si,
    x86_reg_di,
    x86_reg_si,
    x86_reg_di,
    x86_reg_end,
    x86_reg_end,
    x86_reg_end,
    x86_reg_end,
};

uint16_t reg_x86[0x10000] = {0}, reg_x86_size[0x10000] = {0};

uint64_t x86_read_reg(cpu_t *cpu, uint16_t reg)
{
    if (reg >= x86_reg_end)
        return 0;
    return memory_little_endian_read(&cpu->regs[reg_x86[reg]], reg_x86_size[reg]);
}

void x86_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value)
{
    if (reg >= x86_reg_end)
        return;
    if (reg >= x86_reg_gs && reg <= x86_reg_ss)
        *(uint32_t *)&cpu->cache[x86_cache_seg_gs + (reg << 2)] = value << 4;
    return memory_little_endian_write(&cpu->regs[reg_x86[reg]], reg_x86_size[reg], value);
}

static inline uint64_t x86_read_pc(cpu_t *cpu, uint8_t size)
{
    uint64_t ret = memory_read(cpu->pc_base + cpu->pc, size, 0);
    cpu->pc += size;
    return ret;
}

static inline int64_t x86_sread_pc(cpu_t *cpu, uint8_t size)
{
    uint64_t ret = x86_read_pc(cpu, size);
    switch (size)
    {
    case 0x01:
        return (int8_t)ret;
    case 0x02:
        return (int16_t)ret;
    case 0x04:
        return (int32_t)ret;
    }
    return ret;
}

static inline void x86_push_reg(cpu_t *cpu, uint16_t stack_reg, uint16_t reg, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) - size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    memory_write(full_address, x86_read_reg(cpu, reg), size, 0);
    x86_write_reg(cpu, stack_reg, address);
}

static inline void x86_push_int(cpu_t *cpu, uint16_t stack_reg, uint32_t value, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) - size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    memory_write(full_address, value, size, 0);
    x86_write_reg(cpu, stack_reg, address);
}

static inline uint32_t x86_pop(cpu_t *cpu, uint16_t stack_reg, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) + size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    x86_write_reg(cpu, stack_reg, address);
    return memory_read(full_address - size, size, 0);
}

static inline uint32_t x86_rm_read(cpu_t *cpu, uint8_t mod, uint8_t size)
{
    switch (mod)
    {
    case 0x00 ... 0x02:
        return memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], size, 0);
    case 0x03:
        return x86_read_reg(cpu, cpu->cache[x86_cache_address0]);
    }
    return 0x00;
}

static inline uint32_t x86_rm_read_address(cpu_t *cpu, uint8_t mod)
{
    switch (mod)
    {
    case 0x00 ... 0x02:
        return *(uint32_t *)&cpu->cache[x86_cache_address0];
    case 0x03:
        return x86_read_reg(cpu, cpu->cache[x86_cache_address0]);
    }
    return 0x00;
}

static inline void x86_rm_write(cpu_t *cpu, uint8_t mod, uint32_t value, uint8_t size)
{
    switch (mod)
    {
    case 0x00 ... 0x02:
        memory_write(*(uint32_t *)&cpu->cache[x86_cache_address0], value, size, 0);
        break;
    case 0x03:
        x86_write_reg(cpu, cpu->cache[x86_cache_address0], value);
        break;
    }
}

static inline void x86_cache_decode_rm(cpu_t *cpu, uint8_t *rm, uint8_t *reg, uint8_t *mod)
{
    uint8_t cache = x86_read_pc(cpu, 1);
    *rm = cache & 0x07;
    *reg = (cache >> 3) & 0x07;
    *mod = (cache >> 6) & 0x03;
}

static inline uint64_t x86_decode_rm(cpu_t *cpu, uint8_t rm, uint8_t size)
{
    uint8_t cache;
    switch (size)
    {
    case 0x01 ... 0x02:
        return x86_read_reg(cpu, x86_rm_reg16_x[rm]) + x86_read_reg(cpu, x86_rm_reg16_y[rm]);
    case 0x04:
        if (rm == 0x04)
        {
            cache = x86_read_pc(cpu, 1);
            switch (cache)
            {
            case 0x08:
                return x86_read_reg(cpu, x86_reg_eax) +
                       x86_read_reg(cpu, x86_reg_ecx);
            case 0x24:
                return x86_read_reg(cpu, x86_reg_esp);
            case 0x88:
                return x86_read_reg(cpu, x86_reg_eax) +
                       x86_read_reg(cpu, x86_reg_ecx) * 4;
            }
        }
        return x86_read_reg(cpu, x86_rm_reg32_x[rm]);
    }
    return 0;
}

static inline void _x86_decode(cpu_t *cpu, uint8_t rm, uint8_t mod, uint8_t size, bool dword_operand)
{
    if (cpu->override & x86_override_dword_address)
    {
        switch (mod)
        {
        case 0x00 ... 0x02:
            size = 4;
            break;
        }
    }
    switch (mod)
    {
    case 0x00:
        switch (rm)
        {
        case 0x06:
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_pc(cpu, size < 2 ? 2 : size);
            break;
        default:
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_decode_rm(cpu, rm, size);
            break;
        }
        return;
    case 0x01:
        *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_decode_rm(cpu, rm, size) +
                                                       x86_sread_pc(cpu, 1);
        return;
    case 0x02:
        *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_decode_rm(cpu, rm, size) +
                                                       x86_sread_pc(cpu, size < 2 ? 2 : size);
        return;
    case 0x03:
        switch (size)
        {
        case 0x01:
            cpu->cache[x86_cache_address0] = x86_regs8[rm];
            break;
        case 0x02:
            if (dword_operand && cpu->override & x86_override_dword_operand)
            {
                cpu->cache[x86_cache_address0] = x86_regs32[rm];
            }
            else
            {
                cpu->cache[x86_cache_address0] = x86_regs16[rm];
            }
            break;
        case 0x04:
            cpu->cache[x86_cache_address0] = x86_regs32[rm];
            break;
        }
        return;
    }
}

static inline void x86_decode(cpu_t *cpu, uint8_t rm, uint8_t mod, uint8_t size)
{
    _x86_decode(cpu, rm, mod, size, true);
}

static inline void x86_decode_no_operand(cpu_t *cpu, uint8_t rm, uint8_t mod, uint8_t size)
{
    _x86_decode(cpu, rm, mod, size, false);
}

static inline void x86_test(cpu_t *cpu, uint64_t value1, uint64_t value2, uint8_t size)
{
    uint32_t flags;
    flags = x86_read_reg(cpu, x86_reg_eflags);
    if (value1 & value2)
        flags |= x86_flags_ZF;
    else
        flags &= ~x86_flags_ZF;
    x86_write_reg(cpu, x86_reg_eflags, flags);
}

static inline void x86_cmp(cpu_t *cpu, uint64_t value1, uint64_t value2, uint8_t size)
{
    uint32_t flags;
    flags = x86_read_reg(cpu, x86_reg_eflags);
    if (value1 == value2)
        flags |= x86_flags_ZF;
    else
        flags &= ~x86_flags_ZF;
    if (value1 < value2)
        flags |= x86_flags_CF;
    else
        flags &= ~x86_flags_CF;
    x86_write_reg(cpu, x86_reg_eflags, flags);
}

static inline void x86_jump_far(cpu_t *cpu, uint32_t value, uint16_t segment)
{
    x86_write_reg(cpu, x86_reg_cs, segment);
    cpu->pc = value;
}

static inline void x86_jumpif_near(cpu_t *cpu, uint16_t flags, int64_t value)
{
    if (x86_read_reg(cpu, x86_reg_eflags) & flags)
        cpu->pc += value;
}

static inline void x86_jumpNotif_near(cpu_t *cpu, uint16_t flags, int64_t value)
{
    if (~x86_read_reg(cpu, x86_reg_eflags) & flags)
        cpu->pc += value;
}

static inline void x86_opcode_90_97(cpu_t *cpu)
{
    uint8_t reg = cpu->cache[x86_cache_opcode] & 0x07;
    uint64_t reg_value;
    if (cpu->override & x86_override_dword_operand)
    {
        reg_value = x86_read_reg(cpu, x86_regs32[reg]);
        x86_write_reg(cpu, x86_regs32[reg], x86_read_reg(cpu, x86_reg_eax));
        x86_write_reg(cpu, x86_reg_eax, reg_value);
    }
    else
    {
        reg_value = x86_read_reg(cpu, x86_regs16[reg]);
        x86_write_reg(cpu, x86_regs16[reg], x86_read_reg(cpu, x86_reg_ax));
        x86_write_reg(cpu, x86_reg_ax, reg_value);
    }
}

static inline void x86_opcode_8e(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    x86_decode_no_operand(cpu, rm, mod, 2);
    x86_write_reg(cpu, x86_sregs[reg], x86_rm_read(cpu, mod, 2));
}

static inline void x86_opcode_8c(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    x86_decode_no_operand(cpu, rm, mod, 2);
    x86_rm_write(cpu, mod, x86_read_reg(cpu, x86_sregs[reg]), 2);
}

static inline void x86_opcode_8a_8b(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu, x86_regs32[reg], x86_rm_read(cpu, mod, 4));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu, x86_regs16[reg], x86_rm_read(cpu, mod, 2));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu, x86_regs8[reg], x86_rm_read(cpu, mod, 1));
    }
}

static inline void x86_opcode_88_89(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod, x86_read_reg(cpu, x86_regs32[reg]), 4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod, x86_read_reg(cpu, x86_regs16[reg]), 2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod, x86_read_reg(cpu, x86_regs8[reg]), 1);
    }
}

static inline void x86_opcode_84_85(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_test(cpu, x86_rm_read(cpu, mod, 4),
                     x86_read_reg(cpu, x86_regs32[reg]), 4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_test(cpu, x86_rm_read(cpu, mod, 2),
                     x86_read_reg(cpu, x86_regs16[reg]), 2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_test(cpu, x86_rm_read(cpu, mod, 1),
                 x86_read_reg(cpu, x86_regs8[reg]), 1);
    }
}

static inline void x86_opcode_80_83(cpu_t *cpu)
{
    uint8_t type = cpu->cache[x86_cache_opcode] & 0x03, ts0, ts1;
    uint8_t b8083[4][4] = {
        {1, 2, 2, 2},
        {1, 2, 2, 1},
        {1, 2, 2, 2},
        {1, 4, 4, 1},
    };
    ts0 = b8083[(cpu->override & x86_override_dword_operand ? 2 : 0) + 0][type];
    ts1 = b8083[(cpu->override & x86_override_dword_operand ? 2 : 0) + 1][type];
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    x86_decode(cpu, rm, mod, ts0);
    int32_t value = x86_sread_pc(cpu, ts1);
    switch (reg)
    {
    case 0x00:
        x86_rm_write(cpu, mod, x86_rm_read(cpu, mod, ts1) + value, ts1);
        break;
    case 0x01:
        x86_rm_write(cpu, mod, x86_rm_read(cpu, mod, ts1) | value, ts1);
        break;
    case 0x05:
        x86_rm_write(cpu, mod, x86_rm_read(cpu, mod, ts1) - value, ts1);
        break;
    case 0x07:
        x86_cmp(cpu, x86_rm_read(cpu, mod, ts1), value, ts1);
        break;
    }
}

static inline void x86_opcode_b0_bf(cpu_t *cpu, uint8_t size)
{
    uint8_t reg = cpu->cache[x86_cache_opcode] & 0x07;
    if (cpu->override & x86_override_dword_operand)
        size = 4;
    switch (size)
    {
    case 0x01:
        x86_write_reg(cpu, x86_regs8[reg], x86_read_pc(cpu, 1));
        break;
    case 0x02:
        x86_write_reg(cpu, x86_regs16[reg], x86_read_pc(cpu, 2));
        break;
    case 0x04:
        x86_write_reg(cpu, x86_regs32[reg], x86_read_pc(cpu, 4));
        break;
    }
}

static inline void x86_opcode_a0_a1(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_address)
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_pc(cpu, 4);
        else
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_pc(cpu, 2);
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 4, 0));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 2, 0));
    }
    else
    {
        if (cpu->override & x86_override_dword_address)
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_pc(cpu, 4);
        else
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_pc(cpu, 1);
        x86_write_reg(cpu, x86_reg_al,
                      memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 1, 0));
    }
}

static inline void x86_opcode_c6_c7(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_read_pc(cpu, 4),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_read_pc(cpu, 2),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_read_pc(cpu, cpu->override & x86_override_dword_address ? 4 : 1),
                     1);
    }
}

static inline void x86_opcode_50_57(cpu_t *cpu)
{
    if (cpu->override & x86_override_dword_operand)
        x86_push_reg(cpu, x86_reg_esp,
                     x86_regs32[cpu->cache[x86_cache_opcode] & 0x07], 4);
    else
        x86_push_reg(cpu, x86_reg_esp,
                     x86_regs16[cpu->cache[x86_cache_opcode] & 0x07], 2);
}

static inline void x86_opcode_58_5f(cpu_t *cpu)
{
    if (cpu->override & x86_override_dword_operand)
        x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_opcode] & 0x07],
                      x86_pop(cpu, x86_reg_esp, 4));
    else
        x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_opcode] & 0x07],
                      x86_pop(cpu, x86_reg_esp, 2));
}

static inline void x86_opcode_48_4f(cpu_t *cpu)
{
    uint8_t cache_reg = cpu->cache[x86_cache_opcode] & 0x07;
    if (cpu->override & x86_override_dword_operand)
        x86_write_reg(cpu, x86_regs32[cache_reg], x86_read_reg(cpu, x86_regs32[cache_reg]) - 1);
    else
        x86_write_reg(cpu, x86_regs16[cache_reg], x86_read_reg(cpu, x86_regs16[cache_reg]) - 1);
}

static inline void x86_opcode_40_47(cpu_t *cpu)
{
    uint8_t cache_reg = cpu->cache[x86_cache_opcode] & 0x07;
    if (cpu->override & x86_override_dword_operand)
        x86_write_reg(cpu, x86_regs32[cache_reg], x86_read_reg(cpu, x86_regs32[cache_reg]) + 1);
    else
        x86_write_reg(cpu, x86_regs16[cache_reg], x86_read_reg(cpu, x86_regs16[cache_reg]) + 1);
}

static inline void x86_opcode_3c_3d(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_cmp(cpu, x86_read_reg(cpu, x86_reg_eax), x86_read_pc(cpu, 4), 4);
        else
            x86_cmp(cpu, x86_read_reg(cpu, x86_reg_ax), x86_read_pc(cpu, 2), 2);
    }
    else
    {
        x86_cmp(cpu, x86_read_reg(cpu, x86_reg_al), x86_read_pc(cpu, 1), 1);
    }
}

static inline void x86_opcode_3a_3b(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_cmp(cpu, x86_read_reg(cpu, x86_regs32[reg]), x86_rm_read(cpu, mod, 4), 4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_cmp(cpu, x86_read_reg(cpu, x86_regs16[reg]), x86_rm_read(cpu, mod, 2), 2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_cmp(cpu, x86_read_reg(cpu, x86_regs8[reg]), x86_rm_read(cpu, mod, 1), 1);
    }
}

static inline void x86_opcode_38_39(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_cmp(cpu, x86_rm_read(cpu, mod, 4), x86_read_reg(cpu, x86_regs32[reg]), 4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_cmp(cpu, x86_rm_read(cpu, mod, 2), x86_read_reg(cpu, x86_regs16[reg]), 2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_cmp(cpu, x86_rm_read(cpu, mod, 1), x86_read_reg(cpu, x86_regs8[reg]), 1);
    }
}

static inline void x86_opcode_34_35(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) ^
                              x86_read_pc(cpu, 4));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) ^
                              x86_read_pc(cpu, 2));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) ^
                          x86_read_pc(cpu, 1));
    }
}

static inline void x86_opcode_32_33(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) ^
                              x86_rm_read(cpu, mod, 4));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) ^
                              x86_rm_read(cpu, mod, 2));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) ^
                          x86_rm_read(cpu, mod, 1));
    }
}

static inline void x86_opcode_30_31(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) ^
                             x86_read_reg(cpu, x86_regs32[reg]),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) ^
                             x86_read_reg(cpu, x86_regs16[reg]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) ^
                         x86_read_reg(cpu, x86_regs8[reg]),
                     1);
    }
}

static inline void x86_opcode_2c_2d(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) -
                              x86_read_pc(cpu, 4));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) -
                              x86_read_pc(cpu, 2));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) -
                          x86_read_pc(cpu, 1));
    }
}

static inline void x86_opcode_2a_2b(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) -
                              x86_rm_read(cpu, mod, 4));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) -
                              x86_rm_read(cpu, mod, 2));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) -
                          x86_rm_read(cpu, mod, 1));
    }
}

static inline void x86_opcode_28_29(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) -
                             x86_read_reg(cpu, x86_regs32[reg]),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) -
                             x86_read_reg(cpu, x86_regs16[reg]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) -
                         x86_read_reg(cpu, x86_regs8[reg]),
                     1);
    }
}

static inline void x86_opcode_24_25(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) &
                              x86_read_pc(cpu, 4));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) &
                              x86_read_pc(cpu, 2));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) &
                          x86_read_pc(cpu, 1));
    }
}

static inline void x86_opcode_22_23(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) &
                              x86_rm_read(cpu, mod, 4));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) &
                              x86_rm_read(cpu, mod, 2));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) &
                          x86_rm_read(cpu, mod, 1));
    }
}

static inline void x86_opcode_20_21(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) &
                             x86_read_reg(cpu, x86_regs32[reg]),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) &
                             x86_read_reg(cpu, x86_regs16[reg]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) &
                         x86_read_reg(cpu, x86_regs8[reg]),
                     1);
    }
}

static inline void x86_opcode_1c_1d(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) -
                              x86_read_pc(cpu, 4) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) -
                              x86_read_pc(cpu, 2) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) -
                          x86_read_pc(cpu, 1) +
                          ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
    }
}

static inline void x86_opcode_1a_1b(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) -
                              x86_rm_read(cpu, mod, 4) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) -
                              x86_rm_read(cpu, mod, 2) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) -
                          x86_rm_read(cpu, mod, 1) +
                          ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
    }
}

static inline void x86_opcode_18_19(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) -
                             x86_read_reg(cpu, x86_regs32[reg]) +
                             ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) -
                             x86_read_reg(cpu, x86_regs16[reg]) +
                             ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) -
                         x86_read_reg(cpu, x86_regs8[reg]) +
                         ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01),
                     1);
    }
}

static inline void x86_opcode_14_15(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) +
                              x86_read_pc(cpu, 4) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) +
                              x86_read_pc(cpu, 2) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) +
                          x86_read_pc(cpu, 1) +
                          ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
    }
}

static inline void x86_opcode_12_13(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) +
                              x86_rm_read(cpu, mod, 4) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) +
                              x86_rm_read(cpu, mod, 2) +
                              ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) +
                          x86_rm_read(cpu, mod, 1) +
                          ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01));
    }
}

static inline void x86_opcode_10_11(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) +
                             x86_read_reg(cpu, x86_regs32[reg]) +
                             ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) +
                             x86_read_reg(cpu, x86_regs16[reg]) +
                             ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) +
                         x86_read_reg(cpu, x86_regs8[reg]) +
                         ((x86_read_reg(cpu, x86_reg_flags) & x86_flags_CF) & 0x01),
                     1);
    }
}

static inline void x86_opcode_0f(cpu_t *cpu)
{
    uint16_t cache_reg;
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    switch (reg)
    {
    case 0x00:
        switch (rm)
        {
        case 0x00:
            x86_jumpif_near(cpu, x86_flags_OF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x01:
            x86_jumpNotif_near(cpu, x86_flags_OF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x02:
            x86_jumpif_near(cpu, x86_flags_CF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x03:
            x86_jumpNotif_near(cpu, x86_flags_CF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x04:
            x86_jumpif_near(cpu, x86_flags_ZF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x05:
            x86_jumpNotif_near(cpu, x86_flags_ZF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x06:
            x86_jumpNotif_near(cpu, x86_flags_CF | x86_flags_ZF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        case 0x07:
            x86_jumpif_near(cpu, x86_flags_CF | x86_flags_ZF, x86_sread_pc(cpu, cpu->cache[x86_cache_size]));
            break;
        default:
            printf("0x%x\n", rm);
        }
        break;
    case 0x04:
        switch (rm)
        {
        case 0x00:
            x86_cache_decode_rm(cpu, &rm, &reg, &mod);
            x86_decode(cpu, rm, mod, 2);
            if (cpu->override & x86_override_dword_operand)
                x86_rm_write(cpu, mod,
                             cpu_read_reg(cpu, x86_regscontrol[reg]),
                             4);
            else
                x86_rm_write(cpu, mod,
                             cpu_read_reg(cpu, x86_regscontrol[reg]),
                             2);
            break;
        case 0x02:
            x86_cache_decode_rm(cpu, &rm, &reg, &mod);
            x86_decode(cpu, rm, mod, 2);
            if (cpu->override & x86_override_dword_operand)
                x86_write_reg(cpu,
                              x86_regscontrol[reg],
                              x86_rm_read(cpu, mod, 4));
            else
                x86_write_reg(cpu,
                              x86_regscontrol[reg],
                              x86_rm_read(cpu, mod, 2));
            break;
        }
        break;
    case 0x05:
        x86_cache_decode_rm(cpu, &rm, &reg, &mod);
        if (cpu->override & x86_override_dword_operand)
            cpu->cache[x86_cache_address1] = 4, cache_reg = x86_regs32[reg];
        else
            cpu->cache[x86_cache_address1] = 2, cache_reg = x86_regs16[reg];
        x86_decode(cpu, rm, mod, cpu->cache[x86_cache_address1]);
        x86_write_reg(cpu,
                      cache_reg,
                      x86_read_reg(cpu, cache_reg) *
                          x86_rm_read(cpu, mod, cpu->cache[x86_cache_address1]));
        break;
    case 0x06:
        switch (rm)
        {
        case 0x06:
            x86_cache_decode_rm(cpu, &rm, &reg, &mod);
            x86_decode_no_operand(cpu, rm, mod, 1);
            if (cpu->override & x86_override_dword_operand)
                x86_write_reg(cpu, x86_regs32[reg], x86_rm_read(cpu, mod, 1));
            else
                x86_write_reg(cpu, x86_regs16[reg], x86_rm_read(cpu, mod, 1));
            break;
        case 0x07:
            x86_cache_decode_rm(cpu, &rm, &reg, &mod);
            x86_decode_no_operand(cpu, rm, mod, 2);
            if (cpu->override & x86_override_dword_operand)
                x86_write_reg(cpu, x86_regs32[reg], x86_rm_read(cpu, mod, 2));
            else
                x86_write_reg(cpu, x86_regs16[reg], x86_rm_read(cpu, mod, 2));
            break;
        }
        break;
    }
}

static inline void x86_opcode_0c_0d(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) |
                              x86_read_pc(cpu, 4));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) |
                              x86_read_pc(cpu, 2));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) |
                          x86_read_pc(cpu, 1));
    }
}

static inline void x86_opcode_0a_0b(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) |
                              x86_rm_read(cpu, mod, 4));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) |
                              x86_rm_read(cpu, mod, 2));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) |
                          x86_rm_read(cpu, mod, 1));
    }
}

static inline void x86_opcode_08_09(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) |
                             x86_read_reg(cpu, x86_regs32[reg]),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) |
                             x86_read_reg(cpu, x86_regs16[reg]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) |
                         x86_read_reg(cpu, x86_regs8[reg]),
                     1);
    }
}

static inline void x86_opcode_04_05(cpu_t *cpu)
{
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax,
                          x86_read_reg(cpu, x86_reg_eax) +
                              x86_read_pc(cpu, 4));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          x86_read_reg(cpu, x86_reg_ax) +
                              x86_read_pc(cpu, 2));
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al,
                      x86_read_reg(cpu, x86_reg_al) +
                          x86_read_pc(cpu, 1));
    }
}

static inline void x86_opcode_02_03(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu,
                          x86_regs32[reg],
                          x86_read_reg(cpu, x86_regs32[reg]) +
                              x86_rm_read(cpu, mod, 4));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu,
                          x86_regs16[reg],
                          x86_read_reg(cpu, x86_regs16[reg]) +
                              x86_rm_read(cpu, mod, 2));
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_write_reg(cpu,
                      x86_regs8[reg],
                      x86_read_reg(cpu, x86_regs8[reg]) +
                          x86_rm_read(cpu, mod, 1));
    }
}

static inline void x86_opcode_00_01(cpu_t *cpu)
{
    uint8_t rm = 0, reg = 0, mod = 0;
    x86_cache_decode_rm(cpu, &rm, &reg, &mod);
    if (cpu->override & x86_override_is_word)
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 4) +
                             x86_read_reg(cpu, x86_regs32[reg]),
                         4);
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_rm_write(cpu, mod,
                         x86_rm_read(cpu, mod, 2) +
                             x86_read_reg(cpu, x86_regs16[reg]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, rm, mod, 1);
        x86_rm_write(cpu, mod,
                     x86_rm_read(cpu, mod, 1) +
                         x86_read_reg(cpu, x86_regs8[reg]),
                     1);
    }
}

uint8_t x86_emulate(cpu_t *cpu)
{
    cpu->pc_base = *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
    uint8_t ret = 0, opcode = 0, bit_check = 0;
    uint8_t rm = 0, reg = 0, mod = 0;
    uint16_t old_override = cpu->override;
    if (x86_read_reg(cpu, x86_reg_cr0) & x86_cr0_PE)
    {
        cpu->cache[x86_cache_size] = 4;
        cpu->override = (1 << 0) | (1 << 1);
    }
    else
    {
        cpu->cache[x86_cache_size] = 2;
        cpu->override = 0;
    }
start:
    opcode = x86_read_pc(cpu, 1);
    bit_check = 1 << (opcode & 0x01);
    cpu->cache[x86_cache_opcode] = opcode;
    if (opcode & 0x01)
        cpu->override |= x86_override_is_word;
    else
        cpu->override &= ~x86_override_is_word;
    switch (cpu->cache[x86_cache_opcode])
    {
    case 0x00 ... 0x01:
        x86_opcode_00_01(cpu);
        break;
    case 0x02 ... 0x03:
        x86_opcode_02_03(cpu);
        break;
    case 0x04 ... 0x05:
        x86_opcode_04_05(cpu);
        break;
    case 0x08 ... 0x09:
        x86_opcode_08_09(cpu);
        break;
    case 0x0a ... 0x0b:
        x86_opcode_0a_0b(cpu);
        break;
    case 0x0c ... 0x0d:
        x86_opcode_0c_0d(cpu);
        break;
    case 0x0f:
        x86_opcode_0f(cpu);
        break;
    case 0x10 ... 0x11:
        x86_opcode_10_11(cpu);
        break;
    case 0x12 ... 0x13:
        x86_opcode_12_13(cpu);
        break;
    case 0x14 ... 0x15:
        x86_opcode_14_15(cpu);
        break;
    case 0x18 ... 0x19:
        x86_opcode_18_19(cpu);
        break;
    case 0x1a ... 0x1b:
        x86_opcode_1a_1b(cpu);
        break;
    case 0x1c ... 0x1d:
        x86_opcode_1c_1d(cpu);
        break;
    case 0x20 ... 0x21:
        x86_opcode_20_21(cpu);
        break;
    case 0x22 ... 0x23:
        x86_opcode_22_23(cpu);
        break;
    case 0x24 ... 0x25:
        x86_opcode_24_25(cpu);
        break;
    case 0x28 ... 0x29:
        x86_opcode_28_29(cpu);
        break;
    case 0x2a ... 0x2b:
        x86_opcode_2a_2b(cpu);
        break;
    case 0x2c ... 0x2d:
        x86_opcode_2c_2d(cpu);
        break;
    case 0x30 ... 0x31:
        x86_opcode_30_31(cpu);
        break;
    case 0x32 ... 0x33:
        x86_opcode_32_33(cpu);
        break;
    case 0x34 ... 0x35:
        x86_opcode_34_35(cpu);
        break;
    case 0x38 ... 0x39:
        x86_opcode_38_39(cpu);
        break;
    case 0x3a ... 0x3b:
        x86_opcode_3a_3b(cpu);
        break;
    case 0x3c ... 0x3d:
        x86_opcode_3c_3d(cpu);
        break;
    case 0x40 ... 0x47:
        x86_opcode_40_47(cpu);
        break;
    case 0x48 ... 0x4f:
        x86_opcode_48_4f(cpu);
        break;
    case 0x50 ... 0x57:
        x86_opcode_50_57(cpu);
        break;
    case 0x58 ... 0x5f:
        x86_opcode_58_5f(cpu);
        break;
    case 0x66 ... 0x67:
        if (cpu->override & bit_check)
            cpu->override &= ~bit_check;
        else
            cpu->override |= bit_check;
        goto start;
    case 0x70:
        x86_jumpif_near(cpu, x86_flags_OF, x86_sread_pc(cpu, 1));
        break;
    case 0x71:
        x86_jumpNotif_near(cpu, x86_flags_OF, x86_sread_pc(cpu, 1));
        break;
    case 0x72:
        x86_jumpif_near(cpu, x86_flags_CF, x86_sread_pc(cpu, 1));
        break;
    case 0x73:
        x86_jumpNotif_near(cpu, x86_flags_CF, x86_sread_pc(cpu, 1));
        break;
    case 0x74:
        x86_jumpif_near(cpu, x86_flags_ZF, x86_sread_pc(cpu, 1));
        break;
    case 0x75:
        x86_jumpNotif_near(cpu, x86_flags_ZF, x86_sread_pc(cpu, 1));
        break;
    case 0x80 ... 0x81:
        x86_opcode_80_83(cpu);
        break;
    case 0x83:
        x86_opcode_80_83(cpu);
        break;
    case 0x84 ... 0x85:
        x86_opcode_84_85(cpu);
        break;
    case 0x88 ... 0x89:
        x86_opcode_88_89(cpu);
        break;
    case 0x8a ... 0x8b:
        x86_opcode_8a_8b(cpu);
        break;
    case 0x8c:
        x86_opcode_8c(cpu);
        break;
    case 0x8e:
        x86_opcode_8e(cpu);
        break;
    case 0x8d:
        x86_cache_decode_rm(cpu, &rm, &reg, &mod);
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, rm, mod, 4);
            x86_write_reg(cpu, x86_regs32[reg],
                          x86_rm_read_address(cpu, mod));
        }
        else
        {
            x86_decode(cpu, rm, mod, 2);
            x86_write_reg(cpu, x86_regs16[reg],
                          x86_rm_read_address(cpu, mod));
        }
        break;
    case 0x90 ... 0x97:
        x86_opcode_90_97(cpu);
        break;
    case 0xa0 ... 0xa1:
        x86_opcode_a0_a1(cpu);
        break;
    case 0xb0 ... 0xb7:
        x86_opcode_b0_bf(cpu, 1);
        break;
    case 0xb8 ... 0xbf:
        x86_opcode_b0_bf(cpu, 2);
        break;
    case 0xe8:
        if (cpu->override & x86_override_dword_operand)
            cpu->cache[x86_cache_address1] = 4;
        else
            cpu->cache[x86_cache_address1] = 2;
        *(int64_t *)&cpu->cache[x86_cache_address0] = x86_sread_pc(cpu,
                                                                   cpu->cache[x86_cache_address1]);
        x86_push_int(cpu, x86_reg_esp, (uint32_t)cpu->pc, cpu->cache[x86_cache_address1]);
        cpu->pc += *(int64_t *)&cpu->cache[x86_cache_address0];
        break;
    case 0xe9:
        if (cpu->override & x86_override_dword_operand)
            cpu->pc += x86_sread_pc(cpu, 4);
        else
            cpu->pc += x86_sread_pc(cpu, 2);
        break;
    case 0xea:
        if (cpu->override & x86_override_dword_operand)
            x86_jump_far(cpu, x86_read_pc(cpu, 4), x86_read_pc(cpu, 2));
        else
            x86_jump_far(cpu, x86_read_pc(cpu, 2), x86_read_pc(cpu, 2));
        break;
    case 0xeb:
        cpu->pc += x86_sread_pc(cpu, 1);
        break;
    case 0xec:
        x86_write_reg(cpu, x86_reg_al, io_read(x86_read_reg(cpu, x86_reg_dx), 1));
        break;
    case 0xed:
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax, io_read(x86_read_reg(cpu, x86_reg_dx), 4));
        else
            x86_write_reg(cpu, x86_reg_ax, io_read(x86_read_reg(cpu, x86_reg_dx), 2));
        break;
    case 0xee:
        io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_al), 1);
        break;
    case 0xef:
        if (cpu->override & x86_override_dword_operand)
            io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_eax), 4);
        else
            io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_ax), 2);
        break;
    case 0xc3:
        if (cpu->override & x86_override_dword_operand)
            cpu->pc = x86_pop(cpu, x86_reg_esp, 4);
        else
            cpu->pc = x86_pop(cpu, x86_reg_esp, 2);
        break;
    case 0xc6 ... 0xc7:
        x86_opcode_c6_c7(cpu);
        break;
    default:
        cpu->pc--;
        ret++;
        break;
    }
    cpu->override = old_override;
    cpu->cache[x86_cache_buffer_index] = 255;
    x86_write_reg(cpu, x86_reg_ip, cpu->pc);
    return ret;
}

void x86_reset(cpu_t *cpu)
{
    for (size_t i = x86_reg_cr0; i < x86_reg_cr8; i += 4)
        x86_write_reg(cpu, i, 0);
    cpu->cache[x86_cache_buffer_index] = 255;
    x86_write_reg(cpu, x86_reg_gs, 0x0000);
    x86_write_reg(cpu, x86_reg_fs, 0x0000);
    x86_write_reg(cpu, x86_reg_es, 0x0000);
    x86_write_reg(cpu, x86_reg_ds, 0x0000);
    x86_write_reg(cpu, x86_reg_cs, 0xffff);
    x86_write_reg(cpu, x86_reg_ss, 0x0000);
    cpu->pc = 0;
}

cpu_t *x86_setup()
{
    cpu_t *ret = malloc(sizeof(cpu_t));
    if (!ret)
        return (cpu_t *)NULL;
    memset(ret, 0, sizeof(cpu_t));
    ret->regs = malloc(256);
    ret->cache = malloc(x86_cache_end);
    if (!ret->regs || !ret->cache)
    {
        free(ret->regs);
        free(ret->cache);
        free(ret);
        return (cpu_t *)NULL;
    }
    for (uint16_t i = 0; i < 256; i++)
    {
        if (i >= x86_reg_al && i <= x86_reg_bh)
            reg_x86[i] = ((i - x86_reg_al - (i & 0x01)) << 1) + (i & 0x01) + x86_reg_al, reg_x86_size[i] = 1;
        else if (i >= x86_reg_ax && i <= x86_reg_flags)
            reg_x86[i] = ((i - x86_reg_ax) << 1) + x86_reg_al, reg_x86_size[i] = 2;
        else if (i >= x86_reg_eax && i <= x86_reg_eflags)
            reg_x86[i] = (i - x86_reg_eax) + x86_reg_al, reg_x86_size[i] = 4;
        else if (i >= x86_reg_cr0 && i <= x86_reg_cr8)
            reg_x86[i] = (i - x86_reg_cr0) + (reg_x86[x86_reg_eflags] + 4), reg_x86_size[i] = 4;
        else if (i >= x86_reg_gs && i <= x86_reg_ss)
            reg_x86[i] = i, reg_x86_size[i] = 2;
        else
            reg_x86[i] = i, reg_x86_size[i] = 0;
    }
    ret->read_reg = x86_read_reg;
    ret->write_reg = x86_write_reg;
    ret->reset = x86_reset;
    ret->emulate = x86_emulate;
    cpu_reset(ret);
    return ret;
}
