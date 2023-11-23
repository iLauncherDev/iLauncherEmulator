#include "cpu_x86.h"

uint16_t reg_x86[0x10000] = {0}, reg_x86_size[0x10000] = {0};

uint64_t x86_read_reg(cpu_t *cpu, uint16_t reg)
{
    return memory_little_endian_read(&cpu->regs[reg_x86[reg]], reg_x86_size[reg]);
}

void x86_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value)
{
    return memory_little_endian_write(&cpu->regs[reg_x86[reg]], reg_x86_size[reg], value);
}

uint32_t x86_read_cache(cpu_t *cpu, uint8_t size)
{
    uint32_t ret = memory_read(cpu->pc_base + cpu->pc, size, 0);
    cpu->pc += size;
    return ret;
}

void x86_clear_override(cpu_t *cpu, uint16_t override)
{
    cpu->override &= ~override;
}

void x86_push_reg(cpu_t *cpu, uint16_t stack_reg, uint16_t reg, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) - size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    memory_write(full_address, x86_read_reg(cpu, reg), size, 0);
    x86_write_reg(cpu, stack_reg, address);
}

void x86_push_int(cpu_t *cpu, uint16_t stack_reg, uint32_t value, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) - size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    memory_write(full_address, value, size, 0);
    x86_write_reg(cpu, stack_reg, address);
}

uint32_t x86_pop(cpu_t *cpu, uint16_t stack_reg, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) + size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    x86_write_reg(cpu, stack_reg, address);
    return memory_read(full_address - size, size, 0);
}

void x86_jump_near(cpu_t *cpu, uint32_t value, uint8_t size)
{
    cpu->pc_base = *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
    switch (size)
    {
    case 1:
        cpu->pc += (int8_t)value - cpu->pc;
        break;
    case 2:
        cpu->pc += (int16_t)value - cpu->pc;
        break;
    case 4:
        cpu->pc += (int32_t)value - cpu->pc;
        break;
    }
}

uint32_t x86_rm_read(cpu_t *cpu, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00 ... 0x02:
        return memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], size, 0);
    case 0x03:
        return x86_read_reg(cpu, cpu->cache[x86_cache_address0]);
    }
    return 0x00;
}

uint32_t x86_rm_read_address(cpu_t *cpu)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00 ... 0x02:
        return *(uint32_t *)&cpu->cache[x86_cache_address0];
    case 0x03:
        return x86_read_reg(cpu, cpu->cache[x86_cache_address0]);
    }
    return 0x00;
}

void x86_rm_write(cpu_t *cpu, uint32_t value, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00 ... 0x02:
        memory_write(*(uint32_t *)&cpu->cache[x86_cache_address0], value, size, 0);
        break;
    case 0x03:
        x86_write_reg(cpu, cpu->cache[x86_cache_address0], value);
        break;
    }
}

void x86_cache_decode_rm(cpu_t *cpu)
{
    uint8_t cache = x86_read_cache(cpu, 1);
    cpu->cache[x86_cache_opcode] = cache;
    cpu->cache[x86_cache_rm] = cache & 0x07;
    cpu->cache[x86_cache_reg] = (cache >> 3) & 0x07;
    cpu->cache[x86_cache_mod] = cache >> 6;
}

void x86_decode_rm(cpu_t *cpu, uint8_t size)
{
    switch (size)
    {
    case 0x00 ... 0x02:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x00:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_bx) +
                x86_read_reg(cpu, x86_reg_si);
            break;
        case 0x01:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_bx) +
                x86_read_reg(cpu, x86_reg_di);
            break;
        case 0x02:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_bp) +
                x86_read_reg(cpu, x86_reg_si);
            break;
        case 0x03:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_bp) +
                x86_read_reg(cpu, x86_reg_di);
            break;
        case 0x04:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_si);
            break;
        case 0x05:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_di);
            break;
        case 0x06:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_bp);
            break;
        case 0x07:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_bx);
            break;
        }
        break;
    case 0x03 ... 0x04:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x00:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_eax);
            break;
        case 0x04:
            cpu->cache[x86_cache_rm] = x86_read_cache(cpu, 1);
            switch (cpu->cache[x86_cache_rm])
            {
            case 0x08:
                *(uint32_t *)&cpu->cache[x86_cache_address0] =
                    x86_read_reg(cpu, x86_reg_eax) +
                    x86_read_reg(cpu, x86_reg_ecx);
                break;
            case 0x24:
                *(uint32_t *)&cpu->cache[x86_cache_address0] =
                    x86_read_reg(cpu, x86_reg_esp);
                break;
            }
            break;
        case 0x05:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_ebp);
            break;
        }
        break;
    }
}

void x86_decode(cpu_t *cpu, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00:
        if (cpu->override & x86_override_dword_address && size < 4)
            size = 4;
        x86_decode_rm(cpu, size);
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x06:
            switch (size)
            {
            case 0x00 ... 0x02:
                *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_cache(cpu, 2);
                break;
            case 0x03 ... 0x04:
                *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_cache(cpu, 4);
                break;
            }
            break;
        }
        return;
    case 0x01:
        if (cpu->override & x86_override_dword_address && size < 4)
            size = 4;
        x86_decode_rm(cpu, size);
        *(uint32_t *)&cpu->cache[x86_cache_address0] += (int8_t)x86_read_cache(cpu, 1);
        return;
    case 0x02:
        if (cpu->override & x86_override_dword_address && size < 4)
            size = 4;
        x86_decode_rm(cpu, size);
        switch (size)
        {
        case 0x01 ... 0x02:
            *(uint32_t *)&cpu->cache[x86_cache_address0] += (int16_t)x86_read_cache(cpu, 2);
            break;
        case 0x03 ... 0x04:
            *(uint32_t *)&cpu->cache[x86_cache_address0] += (int32_t)x86_read_cache(cpu, 4);
            break;
        }
        return;
    case 0x03:
        switch (size)
        {
        case 0x01:
            cpu->cache[x86_cache_address0] = x86_regs8[cpu->cache[x86_cache_rm]];
            break;
        case 0x02:
            if (cpu->override & x86_override_dword_operand)
                cpu->cache[x86_cache_address0] = x86_regs32[cpu->cache[x86_cache_rm]];
            else
                cpu->cache[x86_cache_address0] = x86_regs16[cpu->cache[x86_cache_rm]];
            break;
        case 0x03 ... 0x04:
            cpu->cache[x86_cache_address0] = x86_regs32[cpu->cache[x86_cache_rm]];
            break;
        }
        return;
    }
}

uint32_t x86_rel(cpu_t *cpu, uint8_t size)
{
    uint32_t value;
    switch (size)
    {
    case 0x01:
        value = (uint8_t)(x86_read_cache(cpu, 1) + cpu->pc);
        break;
    case 0x02:
        value = (uint16_t)(x86_read_cache(cpu, 2) + cpu->pc);
        break;
    case 0x04:
        value = x86_read_cache(cpu, 4) + cpu->pc;
        break;
    default:
        value = 0;
        break;
    }
    return value;
}

uint32_t x86_imm(cpu_t *cpu, uint8_t size)
{
    uint32_t value;
    switch (size)
    {
    case 0x01:
        value = x86_read_cache(cpu, 1);
        break;
    case 0x02:
        value = x86_read_cache(cpu, 2);
        break;
    case 0x04:
        value = x86_read_cache(cpu, 4);
        break;
    default:
        value = 0;
        break;
    }
    return value;
}

int32_t x86_simm(cpu_t *cpu, uint8_t size)
{
    uint32_t value;
    switch (size)
    {
    case 0x00 ... 0x01:
        value = (int8_t)x86_read_cache(cpu, 1);
        break;
    case 0x02:
        value = (int16_t)x86_read_cache(cpu, 2);
        break;
    case 0x04:
        value = x86_read_cache(cpu, 4);
        break;
    default:
        value = 0;
        break;
    }
    return value;
}

void x86_test(cpu_t *cpu, uint64_t value1, uint64_t value2, uint8_t size)
{
    uint16_t flags_reg = size > 2 ? x86_reg_eflags : x86_reg_flags;
    uint32_t flags;
    flags = x86_read_reg(cpu, flags_reg);
    if (!(value1 & value2))
        flags |= x86_flags_ZF;
    else
        flags &= ~x86_flags_ZF;
    x86_write_reg(cpu, flags_reg, flags);
}

void x86_cmp(cpu_t *cpu, uint64_t value1, uint64_t value2, uint8_t size)
{
    uint16_t flags_reg = size > 2 ? x86_reg_eflags : x86_reg_flags;
    uint32_t flags;
    flags = x86_read_reg(cpu, flags_reg);
    if (value1 == value2)
        flags |= x86_flags_ZF;
    else
        flags &= ~x86_flags_ZF;
    if (value1 < value2)
        flags |= x86_flags_CF;
    else
        flags &= ~x86_flags_CF;
    x86_write_reg(cpu, flags_reg, flags);
}

void x86_jumpif_near(cpu_t *cpu, uint16_t flags, uint64_t value, uint8_t size)
{
    uint16_t flags_reg = size > 2 ? x86_reg_eflags : x86_reg_flags;
    if (x86_read_reg(cpu, flags_reg) & flags)
        x86_jump_near(cpu, value, size);
}

void x86_jumpNotif_near(cpu_t *cpu, uint16_t flags, uint64_t value, uint8_t size)
{
    uint16_t flags_reg = size > 2 ? x86_reg_eflags : x86_reg_flags;
    if (~x86_read_reg(cpu, flags_reg) & flags)
        x86_jump_near(cpu, value, size);
}

void x86_reset(cpu_t *cpu)
{
    memset(cpu->regs, 0, x86_reg_end);
    memset(cpu->cache, 0, x86_cache_end);
    x86_write_reg(cpu, x86_reg_cs, (0xfffff - limit(bios_size, (256 * 1024) - 1)) >> 4);
    *(uint32_t *)&cpu->cache[x86_cache_seg_gs] = *(uint16_t *)&cpu->regs[x86_reg_gs] << 4;
    *(uint32_t *)&cpu->cache[x86_cache_seg_fs] = *(uint16_t *)&cpu->regs[x86_reg_fs] << 4;
    *(uint32_t *)&cpu->cache[x86_cache_seg_es] = *(uint16_t *)&cpu->regs[x86_reg_es] << 4;
    *(uint32_t *)&cpu->cache[x86_cache_seg_ds] = *(uint16_t *)&cpu->regs[x86_reg_ds] << 4;
    *(uint32_t *)&cpu->cache[x86_cache_seg_cs] = *(uint16_t *)&cpu->regs[x86_reg_cs] << 4;
    *(uint32_t *)&cpu->cache[x86_cache_seg_ss] = *(uint16_t *)&cpu->regs[x86_reg_ss] << 4;
    cpu->pc_base = *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
}

void x86_opcode_8a_8b(cpu_t *cpu)
{
    x86_cache_decode_rm(cpu);
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, 4);
            x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 4));
        }
        else
        {
            x86_decode(cpu, 2);
            x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 2));
        }
    }
    else
    {
        x86_decode(cpu, 1);
        x86_write_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 1));
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_88_89(cpu_t *cpu)
{
    x86_cache_decode_rm(cpu);
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, 4);
            x86_rm_write(cpu, x86_read_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]]), 4);
        }
        else
        {
            x86_decode(cpu, 2);
            x86_rm_write(cpu, x86_read_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]]), 2);
        }
    }
    else
    {
        x86_decode(cpu, 1);
        x86_rm_write(cpu, x86_read_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]]), 1);
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_84_85(cpu_t *cpu)
{
    x86_cache_decode_rm(cpu);
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, 4);
            x86_test(cpu, x86_rm_read(cpu, 4),
                     x86_read_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]]), 4);
        }
        else
        {
            x86_decode(cpu, 2);
            x86_test(cpu, x86_rm_read(cpu, 2),
                     x86_read_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]]), 2);
        }
    }
    else
    {
        x86_decode(cpu, 1);
        x86_test(cpu, x86_rm_read(cpu, 1),
                 x86_read_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]]), 1);
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_80_83(cpu_t *cpu)
{
    uint8_t type = cpu->cache[x86_cache_opcode] & 0x03, ts0, ts1;
    uint8_t b8083[2][4] = {
        {1, 2, 2, 2},
        {1, 2, 2, 1},
    };
    ts0 = b8083[0][type], ts1 = b8083[1][type];
    x86_cache_decode_rm(cpu);
    x86_decode(cpu, ts0);
    int32_t value;
    if (ts1 > 1 && cpu->override & x86_override_dword_operand)
        ts1 = 4;
    value = x86_simm(cpu, ts1);
    switch (cpu->cache[x86_cache_reg])
    {
    case 0x00:
        x86_rm_write(cpu, x86_rm_read(cpu, ts1) + value, ts1);
        break;
    case 0x05:
        x86_rm_write(cpu, x86_rm_read(cpu, ts1) - value, ts1);
        break;
    case 0x07:
        x86_cmp(cpu, x86_rm_read(cpu, ts1), value, ts1);
        break;
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_b0_bf(cpu_t *cpu, uint8_t size)
{
    cpu->cache[x86_cache_reg] = cpu->cache[x86_cache_opcode] & 0x07;
    if (cpu->override & x86_override_dword_operand)
        size = 4;
    switch (size)
    {
    case 0x01:
        x86_write_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]], x86_imm(cpu, 1));
        break;
    case 0x02:
        x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_imm(cpu, 2));
        break;
    case 0x04:
        x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_imm(cpu, 4));
        break;
    }
    x86_clear_override(cpu, x86_override_dword_operand);
}

void x86_opcode_a0_a1(cpu_t *cpu)
{
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_address)
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, 4);
        else
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, 2);
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
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, 4);
        else
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, 1);
        x86_write_reg(cpu, x86_reg_al,
                      memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 1, 0));
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_c6_c7(cpu_t *cpu)
{
    x86_cache_decode_rm(cpu);
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, 4);
            x86_rm_write(cpu,
                         x86_imm(cpu, 4),
                         4);
        }
        else
        {
            x86_decode(cpu, 2);
            x86_rm_write(cpu,
                         x86_imm(cpu, 2),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, 1);
        x86_rm_write(cpu,
                     x86_imm(cpu, cpu->override & x86_override_dword_address ? 4 : 1),
                     1);
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_50_57(cpu_t *cpu)
{
    if (cpu->override & x86_override_dword_operand)
        x86_push_reg(cpu, x86_reg_esp,
                     x86_regs32[cpu->cache[x86_cache_opcode] & 0x07], 4);
    else
        x86_push_reg(cpu, x86_reg_sp,
                     x86_regs16[cpu->cache[x86_cache_opcode] & 0x07], 2);
    x86_clear_override(cpu, x86_override_dword_operand);
}

void x86_opcode_58_5f(cpu_t *cpu)
{
    if (cpu->override & x86_override_dword_operand)
        x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_opcode] & 0x07],
                      x86_pop(cpu, x86_reg_esp, 4));
    else
        x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_opcode] & 0x07],
                      x86_pop(cpu, x86_reg_sp, 2));
    x86_clear_override(cpu, x86_override_dword_operand);
}

void x86_opcode_48_4f(cpu_t *cpu)
{
    uint8_t cache_reg = cpu->cache[x86_cache_opcode] & 0x07;
    if (cpu->override & x86_override_dword_operand)
        x86_write_reg(cpu, x86_regs32[cache_reg], x86_read_reg(cpu, x86_regs32[cache_reg]) - 1);
    else
        x86_write_reg(cpu, x86_regs16[cache_reg], x86_read_reg(cpu, x86_regs16[cache_reg]) - 1);
}

void x86_opcode_40_47(cpu_t *cpu)
{
    uint8_t cache_reg = cpu->cache[x86_cache_opcode] & 0x07;
    if (cpu->override & x86_override_dword_operand)
        x86_write_reg(cpu, x86_regs32[cache_reg], x86_read_reg(cpu, x86_regs32[cache_reg]) + 1);
    else
        x86_write_reg(cpu, x86_regs16[cache_reg], x86_read_reg(cpu, x86_regs16[cache_reg]) + 1);
}

void x86_opcode_30_31(cpu_t *cpu)
{
    x86_cache_decode_rm(cpu);
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, 4);
            x86_rm_write(cpu,
                         x86_rm_read(cpu, 4) ^
                             x86_read_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]]),
                         4);
        }
        else
        {
            x86_decode(cpu, 2);
            x86_rm_write(cpu,
                         x86_rm_read(cpu, 2) ^
                             x86_read_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, 1);
        x86_rm_write(cpu,
                     x86_rm_read(cpu, 1) ^
                         x86_read_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]]),
                     1);
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_0f(cpu_t *cpu)
{
    uint16_t cache_reg;
    x86_cache_decode_rm(cpu);
    switch (cpu->cache[x86_cache_reg])
    {
    case 0x00:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x04:
            x86_jumpif_near(cpu, x86_flags_ZF, x86_rel(cpu, cpu->cache[x86_cache_size]), cpu->cache[x86_cache_size]);
            break;
        case 0x05:
            x86_jumpNotif_near(cpu, x86_flags_ZF, x86_rel(cpu, cpu->cache[x86_cache_size]), cpu->cache[x86_cache_size]);
            break;
        default:
            printf("0x%x\n", cpu->cache[x86_cache_rm]);
        }
        break;
    case 0x04:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x00:
            x86_cache_decode_rm(cpu);
            if (cpu->override & x86_override_dword_operand)
                cpu->cache[x86_cache_address1] = 4, cache_reg = x86_regscontrol[cpu->cache[x86_cache_reg]];
            else
                cpu->cache[x86_cache_address1] = 2, cache_reg = x86_regscontrol[cpu->cache[x86_cache_reg]];
            x86_decode(cpu, cpu->cache[x86_cache_address1]);
            x86_rm_write(cpu, cpu_read_reg(cpu, cache_reg), cpu->cache[x86_cache_address1]);
            break;
        case 0x02:
            x86_cache_decode_rm(cpu);
            if (cpu->override & x86_override_dword_operand)
                cpu->cache[x86_cache_address1] = 4, cache_reg = x86_regscontrol[cpu->cache[x86_cache_reg]];
            else
                cpu->cache[x86_cache_address1] = 2, cache_reg = x86_regscontrol[cpu->cache[x86_cache_reg]];
            x86_decode(cpu, cpu->cache[x86_cache_address1]);
            x86_write_reg(cpu, cache_reg, x86_rm_read(cpu, cpu->cache[x86_cache_address1]));
            break;
        }
        break;
    case 0x05:
        x86_cache_decode_rm(cpu);
        if (cpu->override & x86_override_dword_operand)
            cpu->cache[x86_cache_address1] = 4, cache_reg = x86_regs32[cpu->cache[x86_cache_reg]];
        else
            cpu->cache[x86_cache_address1] = 2, cache_reg = x86_regs16[cpu->cache[x86_cache_reg]];
        x86_decode(cpu, cpu->cache[x86_cache_address1]);
        x86_write_reg(cpu,
                      cache_reg,
                      x86_read_reg(cpu, cache_reg) *
                          x86_rm_read(cpu, cpu->cache[x86_cache_address1]));
        x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
        break;
    case 0x06:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x06:
            x86_cache_decode_rm(cpu);
            if (cpu->override & x86_override_dword_operand)
            {
                x86_clear_override(cpu, x86_override_dword_operand), x86_decode(cpu, 1);
                x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 1));
            }
            else
            {
                x86_decode(cpu, 1);
                x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 1));
            }
            x86_clear_override(cpu, x86_override_dword_address);
            break;
        case 0x07:
            x86_cache_decode_rm(cpu);
            if (cpu->override & x86_override_dword_operand)
            {
                x86_clear_override(cpu, x86_override_dword_operand), x86_decode(cpu, 2);
                x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 2));
            }
            else
            {
                x86_decode(cpu, 2);
                x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 2));
            }
            x86_clear_override(cpu, x86_override_dword_address);
            break;
        }
        break;
    }
}

void x86_opcode_04_05(cpu_t *cpu)
{
    if (cpu->cache[x86_cache_is_word])
    {
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax, x86_read_reg(cpu, x86_reg_eax) - x86_imm(cpu, 4));
        else
            x86_write_reg(cpu, x86_reg_ax, x86_read_reg(cpu, x86_reg_ax) - x86_imm(cpu, 2));
        x86_clear_override(cpu, x86_override_dword_operand);
    }
    else
    {
        x86_write_reg(cpu, x86_reg_al, x86_read_reg(cpu, x86_reg_al) - x86_imm(cpu, 1));
    }
}

uint8_t x86_emulate(cpu_t *cpu)
{
    uint8_t ret = 0, opcode = 0, bit_check = 0;
    if (x86_read_reg(cpu, x86_reg_cr0) & x86_cr0_PE)
    {
        cpu->cache[x86_cache_size] = 4;
        cpu->override = (1 << 0) | (1 << 1);
    }
    else
    {
        cpu->cache[x86_cache_size] = 2;
    }
start:
    opcode = x86_read_cache(cpu, 1);
    cpu->cache[x86_cache_opcode] = opcode;
    cpu->cache[x86_cache_is_word] = opcode & 0x01;
    switch (cpu->cache[x86_cache_opcode])
    {
    case 0x04 ... 0x05:
        x86_opcode_04_05(cpu);
        break;
    case 0x0f:
        x86_opcode_0f(cpu);
        break;
    case 0x30 ... 0x31:
        x86_opcode_30_31(cpu);
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
        bit_check = 1 << cpu->cache[x86_cache_is_word];
        if (cpu->override & bit_check)
            cpu->override &= ~bit_check;
        else
            cpu->override |= bit_check;
        goto start;
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
    case 0x8d:
        x86_cache_decode_rm(cpu);
        if (cpu->override & x86_override_dword_operand)
        {
            x86_decode(cpu, 4);
            x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]],
                          x86_rm_read_address(cpu));
        }
        else
        {
            x86_decode(cpu, 2);
            x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]],
                          x86_rm_read_address(cpu));
        }
        x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
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
        *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_rel(cpu,
                                                               cpu->cache[x86_cache_address1]);
        x86_push_int(cpu, x86_reg_sp, (uint32_t)cpu->pc, cpu->cache[x86_cache_address1]);
        x86_jump_near(cpu,
                      *(uint32_t *)&cpu->cache[x86_cache_address0], cpu->cache[x86_cache_address1]);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xe9:
        if (cpu->override & x86_override_dword_operand)
            x86_jump_near(cpu,
                          x86_rel(cpu, 4),
                          4);
        else
            x86_jump_near(cpu,
                          x86_rel(cpu, 2),
                          2);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xeb:
        x86_jump_near(cpu, x86_rel(cpu, 1), 1);
        break;
    case 0xec:
        x86_write_reg(cpu, x86_reg_al, io_read(x86_read_reg(cpu, x86_reg_dx), 1));
        break;
    case 0xed:
        if (cpu->override & x86_override_dword_operand)
            x86_write_reg(cpu, x86_reg_eax, io_read(x86_read_reg(cpu, x86_reg_dx), 4));
        else
            x86_write_reg(cpu, x86_reg_ax, io_read(x86_read_reg(cpu, x86_reg_dx), 2));
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xee:
        io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_al), 1);
        break;
    case 0xef:
        if (cpu->override & x86_override_dword_operand)
            io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_eax), 4);
        else
            io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_ax), 2);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xc3:
        if (cpu->override & x86_override_dword_operand)
            cpu->cache[x86_cache_address1] = 4;
        else
            cpu->cache[x86_cache_address1] = 2;
        x86_jump_near(cpu,
                      x86_pop(cpu, x86_reg_sp, cpu->cache[x86_cache_address1]),
                      cpu->cache[x86_cache_address1]);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xc6 ... 0xc7:
        x86_opcode_c6_c7(cpu);
        break;
    default:
        cpu->pc--;
        ret++;
        break;
    }
    x86_write_reg(cpu, x86_reg_ip, cpu->pc);
    return ret;
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