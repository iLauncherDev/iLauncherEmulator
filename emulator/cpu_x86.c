#include "cpu_x86.h"

global_uint64_t x86_get_address(uint16_t seg, global_uint64_t offset)
{
    return (seg << 4) + offset;
}

global_uint64_t x86_read_reg(cpu_t *cpu, uint16_t reg)
{
    if (reg == x86_reg_gdtr)
    {
        return *(global_uint64_t *)&cpu->regs[reg];
    }
    if (reg >= x86_reg_eax && reg <= x86_reg_r15d)
    {
        uint16_t offset = ((reg - x86_reg_eax) >> 1) + x86_reg_al;
        return (*(uint16_t *)&cpu->regs[((reg - x86_reg_eax) >> 1) + x86_reg_ax] << 16) |
               (*(uint8_t *)&cpu->regs[offset + 1] << 8) | *(uint8_t *)&cpu->regs[offset];
    }
    if (reg >= x86_reg_ax && reg <= x86_reg_r15w)
    {
        uint16_t offset = (reg - x86_reg_ax) + x86_reg_al;
        return (*(uint8_t *)&cpu->regs[offset + 1] << 8) | *(uint8_t *)&cpu->regs[offset];
    }
    if (reg >= x86_reg_al && reg <= x86_reg_r15b)
    {
        return *(uint8_t *)&cpu->regs[reg];
    }
    if (reg >= x86_reg_gs && reg <= x86_reg_ss)
    {
        return *(uint16_t *)&cpu->regs[reg];
    }
    return 0;
}

void x86_write_reg(cpu_t *cpu, uint16_t reg, global_uint64_t value)
{
    if (reg == x86_reg_gdtr)
    {
        *(global_uint64_t *)&cpu->regs[reg] = value;
        return;
    }
    if (reg >= x86_reg_eax && reg <= x86_reg_r15d)
    {
        *(uint16_t *)&cpu->regs[((reg - x86_reg_eax) >> 1) + x86_reg_al] = value & 0xffff;
        *(uint16_t *)&cpu->regs[((reg - x86_reg_eax) >> 1) + x86_reg_ax] = (value >> 16) & 0xffff;
        return;
    }
    if (reg >= x86_reg_ax && reg <= x86_reg_r15w)
    {
        *(uint16_t *)&cpu->regs[(reg - x86_reg_ax) + x86_reg_al] = value & 0xffff;
        return;
    }
    if (reg >= x86_reg_al && reg <= x86_reg_r15b)
    {
        *(uint8_t *)&cpu->regs[reg] = value & 0xff;
        return;
    }
    if (reg >= x86_reg_gs && reg <= x86_reg_ss)
    {
        *(uint16_t *)&cpu->regs[reg] = value & 0xffff;
        return;
    }
}

bool x86_check_override(cpu_t *cpu, uint16_t override)
{
    uint16_t value = (cpu->cache[x86_cache_overrideh] << 8) | cpu->cache[x86_cache_overridel];
    bool is_defined = value & override ? true : false;
    if (is_defined)
        value &= ~override;
    cpu->cache[x86_cache_overridel] = value & 0xff;
    cpu->cache[x86_cache_overrideh] = (value >> 8) & 0xff;
    return is_defined;
}

void x86_push_reg(cpu_t *cpu, uint16_t stack_reg, uint16_t reg, uint8_t size)
{
    global_uint64_t address = cpu_read_reg(cpu, stack_reg) - size,
                    full_address = x86_get_address(cpu_read_reg(cpu, x86_reg_ss), address);
    memory_write(full_address, cpu_read_reg(cpu, reg), size, 0);
    cpu_write_reg(cpu, stack_reg, address);
}

void x86_push_int(cpu_t *cpu, uint16_t stack_reg, global_uint64_t value, uint8_t size)
{
    global_uint64_t address = cpu_read_reg(cpu, stack_reg) - size,
                    full_address = x86_get_address(cpu_read_reg(cpu, x86_reg_ss), address);
    memory_write(full_address, value, size, 0);
    cpu_write_reg(cpu, stack_reg, address);
}

global_uint64_t x86_pop(cpu_t *cpu, uint16_t stack_reg, uint8_t size)
{
    global_uint64_t address = cpu_read_reg(cpu, stack_reg) + size,
                    full_address = x86_get_address(cpu_read_reg(cpu, x86_reg_ss), address);
    cpu_write_reg(cpu, stack_reg, address);
    return memory_read(full_address - size, size, 0);
}

void x86_jump_near(cpu_t *cpu, uint16_t reg, uint64_t value, uint8_t size)
{
    global_uint64_t reg_value = cpu_read_reg(cpu, reg);
    switch (size)
    {
    case 0x00 ... 0x01:
        cpu_write_reg(cpu, reg, reg_value + ((int8_t)value - reg_value));
        break;
    case 0x02:
        cpu_write_reg(cpu, reg, reg_value + ((int16_t)value - reg_value));
        break;
    case 0x04:
        cpu_write_reg(cpu, reg, reg_value + ((int32_t)value - reg_value));
        break;
    default:
        break;
    }
    cpu->pc = x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0) + cpu_read_reg(cpu, reg);
}

global_uint64_t x86_rm_read(cpu_t *cpu, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00 ... 0x02:
        return memory_read(*(global_uint64_t *)&cpu->cache[x86_cache_address], size, 0);
    case 0x03:
        return cpu_read_reg(cpu, *(global_uint64_t *)&cpu->cache[x86_cache_address]);
    }
    return 0x00;
}

void x86_rm_write(cpu_t *cpu, global_uint64_t value, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00 ... 0x02:
        memory_write(*(global_uint64_t *)&cpu->cache[x86_cache_address], value, size, 0);
        break;
    case 0x03:
        cpu_write_reg(cpu, *(global_uint64_t *)&cpu->cache[x86_cache_address], value);
        break;
    }
}

void x86_rm_printf(cpu_t *cpu, uint8_t size)
{
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00 ... 0x02:
        if (cpu->cache[x86_cache_rm] == 0x06 && !cpu->cache[x86_cache_mod])
            printf("[0x%llx]", *(global_uint64_t *)&cpu->cache[x86_cache_address]);
        else
            printf("[%s %c 0x%llx]",
                   x86_rm16_strings[cpu->cache[x86_cache_rm]],
                   *(global_int64_t *)&cpu->cache[x86_cache_address] > -1 ? '+'
                                                                          : '-',
                   *(global_int64_t *)&cpu->cache[x86_cache_address] > -1
                       ? *(global_uint64_t *)&cpu->cache[x86_cache_address]
                       : -(*(global_uint64_t *)&cpu->cache[x86_cache_address]));
        break;
    case 0x03:
        printf("%s", x86_regs_strings[*(global_uint64_t *)&cpu->cache[x86_cache_address]]);
        break;
    }
}

void x86_decode(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    if (x86_check_override(cpu, x86_override_dword_address))
        size = 4;
    uint8_t cache = memory_read(cpu->pc++, 1, 0);
    cpu->cache[x86_cache_rm] = cache & 0x07;
    cpu->cache[x86_cache_mod] = (cache & 0xc0) >> 0x06;
    cpu->cache[x86_cache_reg] = (cache & 0x38) >> 0x03;
    switch (cpu->cache[x86_cache_mod])
    {
    case 0x00:
        switch (cpu->cache[x86_cache_rm])
        {
        case 0x06:
            switch (size)
            {
            case 0x00 ... 0x02:
                *(global_uint64_t *)&cpu->cache[x86_cache_address] = memory_read(cpu->pc, 2, 0);
                cpu->pc += 2;
                break;
            case 0x04:
                *(global_uint64_t *)&cpu->cache[x86_cache_address] = memory_read(cpu->pc, 4, 0);
                cpu->pc += 4;
                break;
            }
            *(global_uint64_t *)&cpu->cache[x86_cache_address_old] =
                *(global_uint64_t *)&cpu->cache[x86_cache_address];
            cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
            return;
        }
        cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
        break;
    case 0x01:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] = (int8_t)memory_read(cpu->pc, 1, 0);
        *(global_uint64_t *)&cpu->cache[x86_cache_address_old] =
            *(global_uint64_t *)&cpu->cache[x86_cache_address];
        cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
        cpu->pc += 1;
        break;
    case 0x02:
        switch (size)
        {
        case 0x00 ... 0x02:
            *(global_uint64_t *)&cpu->cache[x86_cache_address] = (int16_t)memory_read(cpu->pc, 2, 0);
            cpu->pc += 2;
            break;
        case 0x04:
            *(global_uint64_t *)&cpu->cache[x86_cache_address] = (int32_t)memory_read(cpu->pc, 4, 0);
            cpu->pc += 4;
            break;
        }
        *(global_uint64_t *)&cpu->cache[x86_cache_address_old] =
            *(global_uint64_t *)&cpu->cache[x86_cache_address];
        cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
        break;
    case 0x03:
        switch (size)
        {
        case 0x00 ... 0x01:
            *(global_uint64_t *)&cpu->cache[x86_cache_address] = x86_reg_al + cpu->cache[x86_cache_rm];
            break;
        case 0x02:
            *(global_uint64_t *)&cpu->cache[x86_cache_address] = x86_reg_ax + (cpu->cache[x86_cache_rm] << 1);
            break;
        case 0x04:
            *(global_uint64_t *)&cpu->cache[x86_cache_address] = x86_reg_eax + (cpu->cache[x86_cache_rm] << 2);
            break;
        }
        *(global_uint64_t *)&cpu->cache[x86_cache_address_old] =
            *(global_uint64_t *)&cpu->cache[x86_cache_address];
        cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
        return;
    }
    switch (cpu->cache[x86_cache_rm])
    {
    case 0x00:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_bx) +
            cpu_read_reg(cpu, x86_reg_si) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x01:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_bx) +
            cpu_read_reg(cpu, x86_reg_di) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x02:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_bp) +
            cpu_read_reg(cpu, x86_reg_si) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x03:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_bp) +
            cpu_read_reg(cpu, x86_reg_di) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x04:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_si) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x05:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_di) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x06:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_bp) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    case 0x07:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] =
            cpu_read_reg(cpu, x86_reg_bx) +
            *(global_int64_t *)&cpu->cache[x86_cache_address];
        break;
    }
}

global_uint64_t x86_rel(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    cpu->pc += size;
    cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
    switch (size)
    {
    case 0x00 ... 0x01:
        return (uint8_t)(cpu_read_reg(cpu, reg) + memory_read(cpu->pc - size, 1, 0));
    case 0x02:
        return (uint16_t)(cpu_read_reg(cpu, reg) + memory_read(cpu->pc - size, 2, 0));
    case 0x04:
        return (uint32_t)(cpu_read_reg(cpu, reg) + memory_read(cpu->pc - size, 4, 0));
    }
    return 0x00;
}

global_uint64_t x86_srel(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    cpu->pc += size;
    cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
    switch (size)
    {
    case 0x00 ... 0x01:
        return (int8_t)(cpu_read_reg(cpu, reg) + memory_read(cpu->pc - size, 1, 0));
    case 0x02:
        return (int16_t)(cpu_read_reg(cpu, reg) + memory_read(cpu->pc - size, 2, 0));
    case 0x04:
        return (int32_t)(cpu_read_reg(cpu, reg) + memory_read(cpu->pc - size, 4, 0));
    }
    return 0x00;
}

global_uint64_t x86_imm(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    cpu->pc += size;
    cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
    switch (size)
    {
    case 0x00 ... 0x01:
        return (uint8_t)memory_read(cpu->pc - size, 1, 0);
    case 0x02:
        return (uint16_t)memory_read(cpu->pc - size, 2, 0);
    case 0x04:
        return (uint32_t)memory_read(cpu->pc - size, 4, 0);
    }
    return 0x00;
}

global_uint64_t x86_simm(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    cpu->pc += size;
    cpu_write_reg(cpu, reg, cpu->pc - x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0));
    switch (size)
    {
    case 0x00 ... 0x01:
        return (int8_t)memory_read(cpu->pc - size, 1, 0);
    case 0x02:
        return (int16_t)memory_read(cpu->pc - size, 2, 0);
    case 0x04:
        return (int32_t)memory_read(cpu->pc - size, 4, 0);
    }
    return 0x00;
}

void x86_reset(cpu_t *cpu)
{
    memset(cpu->regs, 0, x86_reg_end);
    cpu_write_reg(cpu, x86_reg_cs, (0xfffff - limit(bios_size, (256 * 1024) - 1)) >> 4);
    cpu->pc = x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0);
}

void x86_opcode_88_89(cpu_t *cpu, uint16_t reg)
{
    uint16_t cache_reg;
    if (cpu->cache[x86_cache_opcode] & 0x01)
    {
        x86_decode(cpu, reg, 2);
        if (x86_check_override(cpu, x86_override_dword_operand))
        {
            cache_reg = x86_reg_eax + (cpu->cache[x86_cache_reg] << 2);
            x86_rm_write(cpu, cpu_read_reg(cpu, cache_reg), 4);
        }
        else
        {
            cache_reg = x86_reg_ax + (cpu->cache[x86_cache_reg] << 1);
            x86_rm_write(cpu, cpu_read_reg(cpu, cache_reg), 2);
        }
    }
    else
    {
        x86_decode(cpu, reg, 1);
        cache_reg = x86_reg_al + cpu->cache[x86_cache_reg];
        x86_rm_write(cpu, cpu_read_reg(cpu, cache_reg), 1);
    }
    if (cpu->flags & cpu_flag_debug)
        printf("mov "), x86_rm_printf(cpu, 2), printf(", %s", x86_regs_strings[cache_reg]);
}

void x86_opcode_80_83(cpu_t *cpu, uint16_t reg)
{
    uint8_t b8083[4] = {1, 2, 2, 2};
    uint8_t size = b8083[cpu->cache[x86_cache_opcode] & 0x03];
    x86_decode(cpu, reg, size);
    global_uint64_t value;
    switch (cpu->cache[x86_cache_reg])
    {
    case 0x00:
        value = x86_simm(cpu, reg, size);
        x86_rm_write(cpu, x86_rm_read(cpu, size) + (global_int64_t)value, size);
        if (cpu->flags & cpu_flag_debug)
            printf("add "), x86_rm_printf(cpu, size), printf(", 0x%llx", value);
        break;
    }
}

void x86_opcode_b0_bf(cpu_t *cpu, uint16_t reg, uint8_t size)
{
    uint8_t cache_reg = cpu->cache[x86_cache_opcode] & 0x07;
    if (x86_check_override(cpu, x86_override_dword_operand))
        size = 4;
    switch (size)
    {
    case 0x01:
        cpu_write_reg(cpu, x86_reg_al + cache_reg, x86_imm(cpu, reg, 1));
        if (cpu->flags & cpu_flag_debug)
            printf("mov byte %s, 0x%llx",
                   x86_regs_strings[x86_reg_al + cache_reg],
                   cpu_read_reg(cpu, x86_reg_al + cache_reg));
        break;
    case 0x02:
        cpu_write_reg(cpu, x86_reg_ax + (cache_reg << 1), x86_imm(cpu, reg, 2));
        if (cpu->flags & cpu_flag_debug)
            printf("mov word %s, 0x%llx",
                   x86_regs_strings[x86_reg_ax + (cache_reg << 1)],
                   cpu_read_reg(cpu, x86_reg_ax + (cache_reg << 1)));
        break;
    case 0x04:
        cpu_write_reg(cpu, x86_reg_eax + (cache_reg << 2), x86_imm(cpu, reg, 4));
        if (cpu->flags & cpu_flag_debug)
            printf("mov dword %s, 0x%llx",
                   x86_regs_strings[x86_reg_eax + (cache_reg << 2)],
                   cpu_read_reg(cpu, x86_reg_eax + (cache_reg << 2)));
        break;
    }
}

uint8_t x86_emulate(cpu_t *cpu)
{
    cpu->cache[x86_cache_opcode] = memory_read(cpu->pc++, 1, 0);
    switch (cpu->cache[x86_cache_opcode])
    {
    case 0x66 ... 0x67:
        cpu->cache[x86_cache_overridel] |= 1 << (cpu->cache[x86_cache_opcode] & 0x01);
        return x86_emulate(cpu);
    case 0x80 ... 0x83:
        x86_opcode_80_83(cpu, x86_reg_ip);
        break;
    case 0x88 ... 0x89:
        x86_opcode_88_89(cpu, x86_reg_ip);
        break;
    case 0xb0 ... 0xb7:
        x86_opcode_b0_bf(cpu, x86_reg_ip, 1);
        break;
    case 0xb8 ... 0xbf:
        x86_opcode_b0_bf(cpu, x86_reg_ip, 2);
        break;
    case 0xe8:
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address_old] = 4;
        else
            cpu->cache[x86_cache_address_old] = 2;
        *(global_uint64_t *)&cpu->cache[x86_cache_address] = x86_rel(cpu, x86_reg_ip,
                                                                     cpu->cache[x86_cache_address_old]);
        x86_push_reg(cpu, x86_reg_sp, x86_reg_ip, cpu->cache[x86_cache_address_old]);
        x86_jump_near(cpu, x86_reg_ip,
                      *(global_uint64_t *)&cpu->cache[x86_cache_address], cpu->cache[x86_cache_address_old]);
        if (cpu->flags & cpu_flag_debug)
            printf("call %s 0x%llx",
                   x86_sizes_strings[cpu->cache[x86_cache_address_old]],
                   *(global_uint64_t *)&cpu->cache[x86_cache_address]);
        break;
    case 0xe9:
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address_old] = 4;
        else
            cpu->cache[x86_cache_address_old] = 2;
        *(global_uint64_t *)&cpu->cache[x86_cache_address] = x86_rel(cpu, x86_reg_ip,
                                                                     cpu->cache[x86_cache_address_old]);
        x86_jump_near(cpu, x86_reg_ip,
                      *(global_uint64_t *)&cpu->cache[x86_cache_address], cpu->cache[x86_cache_address_old]);
        if (cpu->flags & cpu_flag_debug)
            printf("jmp %s 0x%llx",
                   x86_sizes_strings[cpu->cache[x86_cache_address_old]],
                   *(global_uint64_t *)&cpu->cache[x86_cache_address]);
        break;
    case 0xeb:
        *(global_uint64_t *)&cpu->cache[x86_cache_address] = x86_rel(cpu, x86_reg_ip, 1);
        x86_jump_near(cpu, x86_reg_ip, *(global_uint64_t *)&cpu->cache[x86_cache_address], 1);
        if (cpu->flags & cpu_flag_debug)
            printf("jmp short 0x%llx",
                   *(global_uint64_t *)&cpu->cache[x86_cache_address]);
        break;
    case 0xc3:
        if (x86_check_override(cpu, x86_override_dword_operand))
            cpu->cache[x86_cache_address_old] = 4;
        else
            cpu->cache[x86_cache_address_old] = 2;
        x86_jump_near(cpu, x86_reg_ip,
                      x86_pop(cpu, x86_reg_sp, cpu->cache[x86_cache_address_old]),
                      cpu->cache[x86_cache_address_old]);
        if (cpu->flags & cpu_flag_debug)
            switch (cpu->cache[x86_cache_address_old])
            {
            case 0x02:
                printf("ret");
                break;
            case 0x04:
                printf("retd");
                break;
            }
        break;
    default:
        return 1;
    }
    if (cpu->flags & cpu_flag_debug)
        printf("\n");
    return 0;
}

cpu_t *x86_setup()
{
    cpu_t *ret = malloc(sizeof(cpu_t));
    if (!ret)
        return (cpu_t *)NULL;
    memset(ret, 0, sizeof(cpu_t));
    ret->regs = malloc(x86_reg_end);
    if (!ret->regs)
    {
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