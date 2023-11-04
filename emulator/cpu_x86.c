#include "cpu_x86.h"

char *x86_regs_strings[x86_reg_end] = {
    "gs",
    (char *)NULL,
    "fs",
    (char *)NULL,
    "es",
    (char *)NULL,
    "ds",
    (char *)NULL,
    "cs",
    (char *)NULL,
    "ss",
    (char *)NULL,

    "al",
    "ah",
    "cl",
    "ch",
    "dl",
    "dh",
    "bl",
    "bh",
    "spl",
    (char *)NULL,
    "bpl",
    (char *)NULL,
    "sil",
    (char *)NULL,
    "dil",
    (char *)NULL,
    "ipl",
    (char *)NULL,
    "flagsl",
    (char *)NULL,
    "r8b",
    (char *)NULL,
    "r9b",
    (char *)NULL,
    "r10b",
    (char *)NULL,
    "r11b",
    (char *)NULL,
    "r12b",
    (char *)NULL,
    "r13b",
    (char *)NULL,
    "r14b",
    (char *)NULL,
    "r15b",
    (char *)NULL,

    "ax",
    (char *)NULL,
    "cx",
    (char *)NULL,
    "dx",
    (char *)NULL,
    "bx",
    (char *)NULL,
    "sp",
    (char *)NULL,
    "bp",
    (char *)NULL,
    "si",
    (char *)NULL,
    "di",
    (char *)NULL,
    "ip",
    (char *)NULL,
    "flags",
    (char *)NULL,
    "r8w",
    (char *)NULL,
    "r9w",
    (char *)NULL,
    "r10w",
    (char *)NULL,
    "r11w",
    (char *)NULL,
    "r12w",
    (char *)NULL,
    "r13w",
    (char *)NULL,
    "r14w",
    (char *)NULL,
    "r15w",
    (char *)NULL,

    "eax",
    (char *)NULL,
    (char *)NULL,
    "ecx",
    (char *)NULL,
    (char *)NULL,
    "edx",
    (char *)NULL,
    (char *)NULL,
    "ebx",
    (char *)NULL,
    (char *)NULL,
    "esp",
    (char *)NULL,
    (char *)NULL,
    "ebp",
    (char *)NULL,
    (char *)NULL,
    "esi",
    (char *)NULL,
    (char *)NULL,
    "edi",
    (char *)NULL,
    (char *)NULL,
    "eip",
    (char *)NULL,
    (char *)NULL,
    "eflags",
    (char *)NULL,
    (char *)NULL,
    "r8d",
    (char *)NULL,
    (char *)NULL,
    "r9d",
    (char *)NULL,
    (char *)NULL,
    "r10d",
    (char *)NULL,
    (char *)NULL,
    "r11d",
    (char *)NULL,
    (char *)NULL,
    "r12d",
    (char *)NULL,
    (char *)NULL,
    "r13d",
    (char *)NULL,
    (char *)NULL,
    "r14d",
    (char *)NULL,
    (char *)NULL,
    "r15d",
    (char *)NULL,
    (char *)NULL,

    "gdtr",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
};

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

void x86_reset(cpu_t *cpu)
{
    memset(cpu->regs, 0, x86_reg_end);
    cpu_write_reg(cpu, x86_reg_cs, (0xfffff - limit(bios_size, (256 * 1024) - 1)) >> 4);
    cpu->pc = x86_get_address(cpu_read_reg(cpu, x86_reg_cs), 0);
}

void x86_opcode_b0_bf(cpu_t *cpu, uint8_t size)
{
    switch (size)
    {
    case 0x01:
        cpu_write_reg(cpu, x86_reg_ip, cpu_read_reg(cpu, x86_reg_ip) + 2);
        cpu_write_reg(cpu, x86_reg_al + cpu->cache[x86_cache_opcode_reg], memory_read(cpu->pc, 1, 0));
        if (cpu->flags & cpu_flag_debug)
            printf("mov %s, 0x%llx\n",
                   x86_regs_strings[x86_reg_al + cpu->cache[x86_cache_opcode_reg]],
                   cpu_read_reg(cpu, x86_reg_al + cpu->cache[x86_cache_opcode_reg]));
        cpu->pc += 1;
        break;
    case 0x02:
        cpu_write_reg(cpu, x86_reg_ip, cpu_read_reg(cpu, x86_reg_ip) + 3);
        cpu_write_reg(cpu, x86_reg_ax + (cpu->cache[x86_cache_opcode_reg] << 1), memory_read(cpu->pc, 2, 0));
        if (cpu->flags & cpu_flag_debug)
            printf("mov %s, 0x%llx\n",
                   x86_regs_strings[x86_reg_ax + (cpu->cache[x86_cache_opcode_reg] << 1)],
                   cpu_read_reg(cpu, x86_reg_ax + (cpu->cache[x86_cache_opcode_reg] << 1)));
        cpu->pc += 2;
        break;
    }
}

uint8_t x86_emulate(cpu_t *cpu)
{
    cpu->cache[x86_cache_opcode] = memory_read(cpu->pc++, 1, 0);
    cpu->cache[x86_cache_opcode_reg] = cpu->cache[x86_cache_opcode] & 0x07;
    switch (cpu->cache[x86_cache_opcode])
    {
    case 0xb0 ... 0xb7:
        x86_opcode_b0_bf(cpu, 1);
        break;
    case 0xb8 ... 0xbf:
        x86_opcode_b0_bf(cpu, 2);
        break;
    default:
        cpu->pc--;
        return 1;
    }
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