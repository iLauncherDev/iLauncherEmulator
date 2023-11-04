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
        return (*(uint16_t *)&cpu->regs[((reg - x86_reg_eax) >> 1) + x86_reg_ax] << 16) |
               *(uint16_t *)&cpu->regs[((reg - x86_reg_eax) >> 1) + x86_reg_al];
    }
    if (reg >= x86_reg_ax && reg <= x86_reg_r15w)
    {
        return *(uint16_t *)&cpu->regs[(reg - x86_reg_ax) + x86_reg_al];
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
}

void x86_emulate(cpu_t *cpu)
{
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