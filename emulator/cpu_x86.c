#include "cpu_x86.h"

uint32_t x86_get_address(uint16_t seg, uint32_t offset)
{
    return (seg << 4) + offset;
}

uint64_t x86_read_reg(cpu_t *cpu, uint16_t reg)
{
    uint8_t size = 0, high = 0;
    if (reg >= x86_reg_al && reg <= x86_reg_bh)
        size = 1, high = reg & 0x01, reg = (reg - x86_reg_al + high) << 1;
    else if (reg >= x86_reg_ax && reg <= x86_reg_flags)
        size = 2, reg = ((reg - x86_reg_ax) << 1) + x86_reg_al;
    else if (reg >= x86_reg_eax && reg <= x86_reg_eflags)
        size = 4, reg = (reg - x86_reg_eax) + x86_reg_al;
    else if (reg >= x86_reg_gs && reg <= x86_reg_ss)
        size = 2;
    return memory_little_endian_read(&cpu->regs[reg + high], size);
}

void x86_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value)
{
    uint8_t size = 0, high = 0;
    if (reg >= x86_reg_al && reg <= x86_reg_bh)
        size = 1, high = reg & 0x01, reg = (reg - x86_reg_al + high) << 1;
    else if (reg >= x86_reg_ax && reg <= x86_reg_flags)
        size = 2, reg = ((reg - x86_reg_ax) << 1) + x86_reg_al;
    else if (reg >= x86_reg_eax && reg <= x86_reg_eflags)
        size = 4, reg = (reg - x86_reg_eax) + x86_reg_al;
    else if (reg >= x86_reg_gs && reg <= x86_reg_ss)
        size = 2;
    return memory_little_endian_write(&cpu->regs[reg + high], size, value);
}

static inline uint32_t x86_read_cache(cpu_t *cpu, uint8_t size)
{
    return memory_read(cpu->pc, size, 0);
}

static inline bool x86_check_override(cpu_t *cpu, uint16_t override)
{
    return *(uint16_t *)&cpu->cache[x86_cache_overridel] & override;
}

static inline void x86_clear_override(cpu_t *cpu, uint16_t override)
{
    *(uint16_t *)&cpu->cache[x86_cache_overridel] &= ~override;
}

static inline void x86_set_override(cpu_t *cpu, uint16_t override)
{
    *(uint16_t *)&cpu->cache[x86_cache_overridel] |= override;
}

static inline void x86_push_reg(cpu_t *cpu, uint16_t stack_reg, uint16_t reg, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) - size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    memory_write(full_address, x86_read_reg(cpu, reg), size, 0);
    x86_write_reg(cpu, stack_reg, address);
}
/*
static inline void x86_push_int(cpu_t *cpu, uint16_t stack_reg, uint32_t value, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) - size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    memory_write(full_address, value, size, 0);
    x86_write_reg(cpu, stack_reg, address);
}
*/
static inline uint32_t x86_pop(cpu_t *cpu, uint16_t stack_reg, uint8_t size)
{
    uint32_t address = x86_read_reg(cpu, stack_reg) + size,
             full_address = *(uint32_t *)&cpu->cache[x86_cache_seg_ss] + address;
    x86_write_reg(cpu, stack_reg, address);
    return memory_read(full_address - size, size, 0);
}

static inline void x86_jump_near(cpu_t *cpu, uint16_t reg, uint32_t value, uint8_t size)
{
    cpu->pc += (*(uint32_t *)&cpu->cache[x86_cache_seg_cs] + value) - cpu->pc;
}

static inline uint32_t x86_rm_read(cpu_t *cpu, uint8_t size)
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

static inline uint32_t x86_rm_read_address(cpu_t *cpu)
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

static inline void x86_rm_write(cpu_t *cpu, uint32_t value, uint8_t size)
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

static inline void x86_cache_decode_rm(cpu_t *cpu, uint16_t reg)
{
    uint8_t cache = x86_read_cache(cpu, 1);
    cpu->cache[x86_cache_rm] = cache & 0x07;
    cpu->cache[x86_cache_reg] = (cache >> 3) & 0x07;
    cpu->cache[x86_cache_mod] = (cache >> 6) & 0x03;
    cpu->pc++;
}

static inline void x86_decode_rm(cpu_t *cpu, uint16_t reg, uint8_t size)
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
            cpu->pc++;
            break;
        case 0x05:
            *(uint32_t *)&cpu->cache[x86_cache_address0] =
                x86_read_reg(cpu, x86_reg_ebp);
            break;
        }
        break;
    }
}

static inline void x86_decode(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00:
        if (x86_check_override(cpu, x86_override_dword_address))
            size = 4;
        x86_decode_rm(cpu, reg, size);
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x06:
            switch (size)
            {
            case 0x00 ... 0x02:
                *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_cache(cpu, 2);
                cpu->pc += 2;
                break;
            case 0x03 ... 0x04:
                *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_read_cache(cpu, 4);
                cpu->pc += 4;
                break;
            }
            break;
        }
        return;
    case 0x01:
        if (x86_check_override(cpu, x86_override_dword_address))
            size = 4;
        x86_decode_rm(cpu, reg, size);
        *(uint32_t *)&cpu->cache[x86_cache_address0] += (int8_t)x86_read_cache(cpu, 1);
        cpu->pc += 1;
        return;
    case 0x02:
        if (x86_check_override(cpu, x86_override_dword_address))
            size = 4;
        x86_decode_rm(cpu, reg, size);
        switch (size)
        {
        case 0x00 ... 0x02:
            *(uint32_t *)&cpu->cache[x86_cache_address0] += (int16_t)x86_read_cache(cpu, 2);
            cpu->pc += 2;
            break;
        case 0x03 ... 0x04:
            *(uint32_t *)&cpu->cache[x86_cache_address0] += (int32_t)x86_read_cache(cpu, 4);
            cpu->pc += 4;
            break;
        }
        return;
    case 0x03:
        switch (size)
        {
        case 0x00 ... 0x01:
            cpu->cache[x86_cache_address0] = x86_regs8[cpu->cache[x86_cache_rm]];
            break;
        case 0x02:
            if (x86_check_override(cpu, x86_override_dword_operand))
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

uint32_t x86_rel(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    uint32_t value, pc = (cpu->pc + size) - *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
    x86_write_reg(cpu, reg, pc);
    switch (size)
    {
    case 0x00 ... 0x01:
        value = (uint8_t)(pc + x86_read_cache(cpu, 1));
        break;
    case 0x02:
        value = (uint16_t)(pc + x86_read_cache(cpu, 2));
        break;
    case 0x04:
        value = (uint32_t)(pc + x86_read_cache(cpu, 4));
        break;
    }
    cpu->pc += size;
    return value;
}

int32_t x86_srel(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    uint32_t value, pc = (cpu->pc + size) - *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
    x86_write_reg(cpu, reg, pc);
    switch (size)
    {
    case 0x00 ... 0x01:
        value = (int8_t)(pc + x86_read_cache(cpu, 1));
        break;
    case 0x02:
        value = (int16_t)(pc + x86_read_cache(cpu, 2));
        break;
    case 0x04:
        value = (int32_t)(pc + x86_read_cache(cpu, 4));
        break;
    }
    cpu->pc += size;
    return value;
}

uint32_t x86_imm(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    uint32_t value, pc = (cpu->pc + size) - *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
    x86_write_reg(cpu, reg, pc);
    switch (size)
    {
    case 0x00 ... 0x01:
        value = x86_read_cache(cpu, 1);
        break;
    case 0x02:
        value = x86_read_cache(cpu, 2);
        break;
    case 0x04:
        value = x86_read_cache(cpu, 4);
        break;
    }
    cpu->pc += size;
    return value;
}

int32_t x86_simm(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    uint32_t value, pc = (cpu->pc + size) - *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
    x86_write_reg(cpu, reg, pc);
    switch (size)
    {
    case 0x00 ... 0x01:
        value = (int8_t)x86_read_cache(cpu, 1);
        break;
    case 0x02:
        value = (int16_t)x86_read_cache(cpu, 2);
        break;
    case 0x04:
        value = (int32_t)x86_read_cache(cpu, 4);
        break;
    }
    cpu->pc += size;
    return value;
}

void x86_cmp(cpu_t *cpu, uint64_t value1, uint64_t value2, uint8_t size)
{
    uint16_t flags_reg;
    uint32_t flags;
    switch (size)
    {
    case 0x00 ... 0x02:
        flags_reg = x86_reg_flags;
        break;
    case 0x03 ... 0x04:
        flags_reg = x86_reg_eflags;
        break;
    }
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

void x86_jumpif_near(cpu_t *cpu, uint16_t reg, uint16_t flags, uint64_t value, uint8_t size)
{
    uint16_t flags_reg;
    switch (size)
    {
    case 0x00 ... 0x02:
        flags_reg = x86_reg_flags;
        break;
    case 0x03 ... 0x04:
        flags_reg = x86_reg_eflags;
        break;
    }
    if (x86_read_reg(cpu, flags_reg) & flags)
        x86_jump_near(cpu, reg, value, size);
}

void x86_jumpNotif_near(cpu_t *cpu, uint16_t reg, uint16_t flags, uint64_t value, uint8_t size)
{
    uint16_t flags_reg;
    switch (size)
    {
    case 0x00 ... 0x02:
        flags_reg = x86_reg_flags;
        break;
    case 0x03 ... 0x04:
        flags_reg = x86_reg_eflags;
        break;
    }
    if (~x86_read_reg(cpu, flags_reg) & flags)
        x86_jump_near(cpu, reg, value, size);
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
    cpu->pc = *(uint32_t *)&cpu->cache[x86_cache_seg_cs];
}

void x86_opcode_8a_8b(cpu_t *cpu, uint16_t reg)
{
    x86_cache_decode_rm(cpu, reg);
    if (cpu->cache[x86_cache_opcode] & 0x01)
    {
        if (x86_check_override(cpu, x86_override_dword_operand))
        {
            x86_decode(cpu, reg, 4);
            x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 4));
        }
        else
        {
            x86_decode(cpu, reg, 2);
            x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 2));
        }
    }
    else
    {
        x86_decode(cpu, reg, 1);
        x86_write_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 1));
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_88_89(cpu_t *cpu, uint16_t reg)
{
    x86_cache_decode_rm(cpu, reg);
    if (cpu->cache[x86_cache_opcode] & 0x01)
    {
        if (x86_check_override(cpu, x86_override_dword_operand))
        {
            x86_decode(cpu, reg, 4);
            x86_rm_write(cpu, x86_read_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]]), 4);
        }
        else
        {
            x86_decode(cpu, reg, 2);
            x86_rm_write(cpu, x86_read_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]]), 2);
        }
    }
    else
    {
        x86_decode(cpu, reg, 1);
        x86_rm_write(cpu, x86_read_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]]), 1);
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_80_83(cpu_t *cpu, uint16_t reg)
{
    uint8_t type = cpu->cache[x86_cache_opcode] & 0x03, ts0, ts1;
    uint8_t b8083[2][4] = {
        {1, 2, 2, 2},
        {1, 2, 1, 1},
    };
    ts0 = b8083[0][type], ts1 = b8083[1][type];
    x86_cache_decode_rm(cpu, reg);
    x86_decode(cpu, reg, ts0);
    int32_t value;
    if (ts1 > 1 && x86_check_override(cpu, x86_override_dword_operand))
        ts1 = 4;
    value = x86_simm(cpu, reg, ts1);
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

void x86_opcode_b0_bf(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    cpu->cache[x86_cache_reg] = cpu->cache[x86_cache_opcode] & 0x07;
    if (x86_check_override(cpu, x86_override_dword_operand))
        size = 4;
    switch (size)
    {
    case 0x01:
        x86_write_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]], x86_imm(cpu, reg, 1));
        break;
    case 0x02:
        x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_imm(cpu, reg, 2));
        break;
    case 0x04:
        x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_imm(cpu, reg, 4));
        break;
    }
    x86_clear_override(cpu, x86_override_dword_operand);
}

void x86_opcode_a0_a1(cpu_t *cpu, uint16_t reg)
{
    if (cpu->cache[x86_cache_opcode] & 0x01)
    {
        if (x86_check_override(cpu, x86_override_dword_address))
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, reg, 4);
        else
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, reg, 2);
        if (x86_check_override(cpu, x86_override_dword_operand))
            x86_write_reg(cpu, x86_reg_eax,
                          memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 4, 0));
        else
            x86_write_reg(cpu, x86_reg_ax,
                          memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 2, 0));
    }
    else
    {
        if (x86_check_override(cpu, x86_override_dword_address))
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, reg, 4);
        else
            *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_imm(cpu, reg, 1);
        x86_write_reg(cpu, x86_reg_al,
                      memory_read(*(uint32_t *)&cpu->cache[x86_cache_address0], 1, 0));
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_50_57(cpu_t *cpu, uint16_t reg)
{
    cpu->cache[x86_cache_reg] = cpu->cache[x86_cache_opcode] & 0x07;
    if (x86_check_override(cpu, x86_override_dword_operand))
        x86_push_reg(cpu, x86_reg_esp,
                     x86_regs32[cpu->cache[x86_cache_reg]], 4);
    else
        x86_push_reg(cpu, x86_reg_sp,
                     x86_regs16[cpu->cache[x86_cache_reg]], 2);
    x86_clear_override(cpu, x86_override_dword_operand);
}

void x86_opcode_58_5f(cpu_t *cpu, uint16_t reg)
{
    cpu->cache[x86_cache_reg] = cpu->cache[x86_cache_opcode] & 0x07;
    if (x86_check_override(cpu, x86_override_dword_operand))
        x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]],
                      x86_pop(cpu, x86_reg_esp, 4));
    else
        x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]],
                      x86_pop(cpu, x86_reg_sp, 2));
    x86_clear_override(cpu, x86_override_dword_operand);
}

void x86_opcode_30_31(cpu_t *cpu, uint16_t reg)
{
    x86_cache_decode_rm(cpu, reg);
    if (cpu->cache[x86_cache_opcode] & 0x01)
    {
        if (x86_check_override(cpu, x86_override_dword_operand))
        {
            x86_decode(cpu, reg, 4);
            x86_rm_write(cpu,
                         x86_rm_read(cpu, 4) ^
                             x86_read_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]]),
                         4);
        }
        else
        {
            x86_decode(cpu, reg, 2);
            x86_rm_write(cpu,
                         x86_rm_read(cpu, 2) ^
                             x86_read_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]]),
                         2);
        }
    }
    else
    {
        x86_decode(cpu, reg, 1);
        x86_rm_write(cpu,
                     x86_rm_read(cpu, 1) ^
                         x86_read_reg(cpu, x86_regs8[cpu->cache[x86_cache_reg]]),
                     1);
    }
    x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
}

void x86_opcode_0f(cpu_t *cpu, uint16_t reg)
{
    uint16_t cache_reg;
    x86_cache_decode_rm(cpu, reg);
    switch (cpu->cache[x86_cache_reg])
    {
    case 0x00:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x04:
            x86_jumpif_near(cpu, reg, x86_flags_ZF, x86_rel(cpu, reg, 2), 2);
            break;
        default:
            printf("0x%x\n", cpu->cache[x86_cache_rm]);
        }
        break;
    case 0x05:
        x86_cache_decode_rm(cpu, reg);
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address1] = 4, cache_reg = x86_regs32[cpu->cache[x86_cache_reg]];
        else
            cpu->cache[x86_cache_address1] = 2, cache_reg = x86_regs16[cpu->cache[x86_cache_reg]];
        x86_decode(cpu, reg, cpu->cache[x86_cache_address1]);
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
            x86_cache_decode_rm(cpu, reg);
            if (x86_check_override(cpu, x86_override_dword_operand))
            {
                x86_decode(cpu, reg, 1);
                x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 1));
            }
            else
            {
                x86_decode(cpu, reg, 1);
                x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 1));
            }
            x86_clear_override(cpu, x86_override_dword_address);
            break;
        case 0x07:
            x86_cache_decode_rm(cpu, reg);
            if (x86_check_override(cpu, x86_override_dword_operand))
            {
                x86_clear_override(cpu, x86_override_dword_operand), x86_decode(cpu, reg, 2);
                x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 2));
            }
            else
            {
                x86_decode(cpu, reg, 2);
                x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]], x86_rm_read(cpu, 2));
            }
            x86_clear_override(cpu, x86_override_dword_address);
            break;
        }
        break;
    }
}

uint8_t x86_emulate(cpu_t *cpu)
{
    uint8_t ret = 0;
start:
    cpu->cache[x86_cache_opcode] = x86_read_cache(cpu, 1), cpu->pc++;
    switch (cpu->cache[x86_cache_opcode])
    {
    case 0x0f:
        x86_opcode_0f(cpu, x86_reg_ip);
        break;
    case 0x30 ... 0x31:
        x86_opcode_30_31(cpu, x86_reg_ip);
        break;
    case 0x50 ... 0x57:
        x86_opcode_50_57(cpu, x86_reg_ip);
        break;
    case 0x58 ... 0x5f:
        x86_opcode_58_5f(cpu, x86_reg_ip);
        break;
    case 0x66:
        x86_set_override(cpu, x86_override_dword_operand);
        goto start;
    case 0x67:
        x86_set_override(cpu, x86_override_dword_address);
        goto start;
    case 0x80 ... 0x83:
        x86_opcode_80_83(cpu, x86_reg_ip);
        break;
    case 0x88 ... 0x89:
        x86_opcode_88_89(cpu, x86_reg_ip);
        break;
    case 0x8a ... 0x8b:
        x86_opcode_8a_8b(cpu, x86_reg_ip);
        break;
    case 0x8d:
        x86_cache_decode_rm(cpu, x86_reg_ip);
        if (x86_check_override(cpu, x86_override_dword_operand))
        {
            x86_decode(cpu, x86_reg_ip, 4);
            x86_write_reg(cpu, x86_regs32[cpu->cache[x86_cache_reg]],
                          x86_rm_read_address(cpu));
        }
        else
        {
            x86_decode(cpu, x86_reg_ip, 2);
            x86_write_reg(cpu, x86_regs16[cpu->cache[x86_cache_reg]],
                          x86_rm_read_address(cpu));
        }
        x86_clear_override(cpu, x86_override_dword_address | x86_override_dword_operand);
        break;
    case 0xa0 ... 0xa1:
        x86_opcode_a0_a1(cpu, x86_reg_ip);
        break;
    case 0xb0 ... 0xb7:
        x86_opcode_b0_bf(cpu, x86_reg_ip, 1);
        break;
    case 0xb8 ... 0xbf:
        x86_opcode_b0_bf(cpu, x86_reg_ip, 2);
        break;
    case 0xe8:
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address1] = 4;
        else
            cpu->cache[x86_cache_address1] = 2;
        *(uint32_t *)&cpu->cache[x86_cache_address0] = x86_rel(cpu, x86_reg_ip,
                                                               cpu->cache[x86_cache_address1]);
        x86_push_reg(cpu, x86_reg_sp, x86_reg_ip, cpu->cache[x86_cache_address1]);
        x86_jump_near(cpu, x86_reg_ip,
                      *(uint32_t *)&cpu->cache[x86_cache_address0], cpu->cache[x86_cache_address1]);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xe9:
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address1] = 4;
        else
            cpu->cache[x86_cache_address1] = 2;
        x86_jump_near(cpu, x86_reg_ip,
                      x86_rel(cpu, x86_reg_ip, cpu->cache[x86_cache_address1]),
                      cpu->cache[x86_cache_address1]);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xeb:
        x86_jump_near(cpu, x86_reg_ip, x86_rel(cpu, x86_reg_ip, 1), 1);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xec:
        x86_write_reg(cpu, x86_reg_al, io_read(x86_read_reg(cpu, x86_reg_dx), 1));
        break;
    case 0xed:
        if (x86_check_override(cpu, x86_override_dword_operand))
            x86_write_reg(cpu, x86_reg_eax, io_read(x86_read_reg(cpu, x86_reg_dx), 4));
        else
            x86_write_reg(cpu, x86_reg_ax, io_read(x86_read_reg(cpu, x86_reg_dx), 2));
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xee:
        io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_al), 1);
        break;
    case 0xef:
        if (x86_check_override(cpu, x86_override_dword_operand))
            io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_eax), 4);
        else
            io_write(x86_read_reg(cpu, x86_reg_dx), x86_read_reg(cpu, x86_reg_ax), 2);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xc3:
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address1] = 4;
        else
            cpu->cache[x86_cache_address1] = 2;
        x86_jump_near(cpu, x86_reg_ip,
                      x86_pop(cpu, x86_reg_sp, cpu->cache[x86_cache_address1]),
                      cpu->cache[x86_cache_address1]);
        x86_clear_override(cpu, x86_override_dword_operand);
        break;
    case 0xc7:
        x86_cache_decode_rm(cpu, x86_reg_ip);
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address1] = 4;
        else
            cpu->cache[x86_cache_address1] = 2;
        x86_decode(cpu, x86_reg_ip, cpu->cache[x86_cache_address1]);
        x86_rm_write(cpu,
                     x86_imm(cpu, x86_reg_ip, cpu->cache[x86_cache_address1]),
                     cpu->cache[x86_cache_address1]);
        x86_clear_override(cpu, x86_override_dword_operand | x86_override_dword_address);
        break;
    default:
        ret++;
        break;
    }
    x86_write_reg(cpu, x86_reg_ip, cpu->pc - *(uint32_t *)&cpu->cache[x86_cache_seg_cs]);
    return ret;
}

cpu_t *x86_setup()
{
    cpu_t *ret = malloc(sizeof(cpu_t));
    if (!ret)
        return (cpu_t *)NULL;
    memset(ret, 0, sizeof(cpu_t));
    ret->regs = malloc(50);
    ret->cache = malloc(x86_cache_end);
    if (!ret->regs || !ret->cache)
    {
        free(ret->regs);
        free(ret->cache);
        free(ret);
        return (cpu_t *)NULL;
    }
    ret->read_reg = x86_read_reg;
    ret->write_reg = x86_write_reg;
    ret->reset = x86_reset;
    ret->emulate = x86_emulate;
    cpu_reset(ret);
    return ret;
}