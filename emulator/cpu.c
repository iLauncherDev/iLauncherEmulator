#include "cpu.h"

extern uint64_t window_framebuffer[];

cpu_info_t cpu_info[8];
uint8_t cpu_info_index = 0;

uint8_t *opcode;
uint8_t cpu_state[cpu_reg_end];
uint8_t regs32[] = {
    cpu_reg_eax,
    cpu_reg_ecx,
    cpu_reg_edx,
    cpu_reg_ebx,
    cpu_reg_esp,
    cpu_reg_ebp,
    cpu_reg_esi,
    cpu_reg_edi,
};
uint8_t regs16[] = {
    cpu_reg_ax,
    cpu_reg_cx,
    cpu_reg_dx,
    cpu_reg_bx,
    cpu_reg_sp,
    cpu_reg_bp,
    cpu_reg_si,
    cpu_reg_di,
};
uint8_t regs8[] = {
    cpu_reg_al,
    cpu_reg_cl,
    cpu_reg_dl,
    cpu_reg_bl,
    cpu_reg_ah,
    cpu_reg_ch,
    cpu_reg_dh,
    cpu_reg_bh,
};
uint8_t segregs[] = {
    cpu_reg_es,
    cpu_reg_cs,
    cpu_reg_ss,
    cpu_reg_ds,
    cpu_reg_fs,
    cpu_reg_gs,
};

uint64_t cpu_read_reg(uint8_t reg)
{
    if (reg == cpu_reg_ip)
        reg = cpu_reg_eip;
    if (reg >= cpu_reg_gdtr && reg < cpu_reg_end && ((reg - cpu_reg_gdtr) & 8) == 0)
    {
        return *(uint64_t *)&cpu_state[reg];
    }
    if (reg >= cpu_reg_eax && reg <= cpu_reg_r15d)
    {
        return (*(uint16_t *)&cpu_state[((reg - cpu_reg_eax) >> 1) + cpu_reg_ax] << 16) |
               *(uint16_t *)&cpu_state[((reg - cpu_reg_eax) >> 1) + cpu_reg_al];
    }
    if (reg >= cpu_reg_ax && reg <= cpu_reg_r15w)
    {
        return *(uint16_t *)&cpu_state[(reg - cpu_reg_ax) + cpu_reg_al];
    }
    if (reg >= cpu_reg_gs && reg <= cpu_reg_r15b)
    {
        return *(uint8_t *)&cpu_state[reg];
    }
    return 0;
}

void cpu_write_reg(uint8_t reg, uint64_t value)
{
    if (reg == cpu_reg_ip)
        reg = cpu_reg_eip;
    if (reg >= cpu_reg_gdtr && reg < cpu_reg_end && ((reg - cpu_reg_gdtr) & 8) == 0)
    {
        *(uint64_t *)&cpu_state[reg] = value;
        return;
    }
    if (reg >= cpu_reg_eax && reg <= cpu_reg_r15d)
    {
        *(uint16_t *)&cpu_state[((reg - cpu_reg_eax) >> 1) + cpu_reg_al] = value & 0xffff;
        *(uint16_t *)&cpu_state[((reg - cpu_reg_eax) >> 1) + cpu_reg_ax] = (value >> 16) & 0xffff;
        return;
    }
    if (reg >= cpu_reg_ax && reg <= cpu_reg_r15w)
    {
        *(uint16_t *)&cpu_state[(reg - cpu_reg_ax) + cpu_reg_al] = value & 0xffff;
        return;
    }
    if (reg >= cpu_reg_gs && reg <= cpu_reg_r15b)
    {
        *(uint8_t *)&cpu_state[reg] = value & 0xff;
        return;
    }
}

static inline void cpu_add_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) + value);
}

static inline void cpu_sub_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) - value);
}

static inline void cpu_mul_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) * value);
}

static inline void cpu_div_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) / value);
}

static inline void cpu_and_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) & value);
}

static inline void cpu_or_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) | value);
}

static inline void cpu_xor_reg(uint8_t reg, uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) ^ value);
}

static inline uint64_t cpu_resolve_value(uint64_t value, uint8_t index, uint8_t size)
{
    gdt_entry_t *entry = (gdt_entry_t *)(&vm_memory[((gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)])->base] +
                                         cpu_read_reg(cpu_reg_ds));
    uint64_t base = (entry->base_high << 24) |
                    (entry->base_middle << 16) |
                    entry->base_low;
    uint64_t buffer_address = 0;
    switch (cpu_info[index].reg_type)
    {
    case cpu_type_memory:
        return memory_read(base + value, size);
    case cpu_type_memory_reg:
        for (uint8_t i = 0; i < cpu_info[index].reg_type_buffer[0]; i++)
            buffer_address += cpu_read_reg(cpu_info[index].reg_type_buffer[i + 1]);
        if (cpu_info[index].reg_type_buffer[0])
            return memory_read(base + (int64_t)value + buffer_address, size);
        else
            return memory_read(base + cpu_read_reg(value), size);
    case cpu_type_reg:
        return cpu_read_reg(value);
    };
    return value;
}

static inline void cpu_exec_mov(uint64_t value1, uint64_t value2, uint8_t size)
{
    value2 = cpu_resolve_value(value2, 1, size);
    gdt_entry_t *entry = (gdt_entry_t *)(&vm_memory[((gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)])->base] +
                                         cpu_read_reg(cpu_reg_ds));
    uint64_t base = (entry->base_high << 24) |
                    (entry->base_middle << 16) |
                    entry->base_low;
    uint64_t buffer_address = 0;
    switch (cpu_info[0].reg_type)
    {
    case cpu_type_memory:
        memory_write(base + value1, value2, size);
        break;
    case cpu_type_memory_reg:
        for (uint8_t i = 0; i < cpu_info[0].reg_type_buffer[0]; i++)
            buffer_address += cpu_read_reg(cpu_info[0].reg_type_buffer[i + 1]);
        if (cpu_info[0].reg_type_buffer[0])
            memory_write(base + (int64_t)value1 + buffer_address, value2, size);
        else
            memory_write(base + cpu_read_reg(value1), value2, size);
        break;
    default:
        cpu_write_reg(value1, value2);
        break;
    };
}

static inline void cpu_exec_add(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_add_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 + value2, size);
}

static inline void cpu_exec_sub(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_sub_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 - value2, size);
}

static inline void cpu_exec_mul(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_mul_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 * value2, size);
}

static inline void cpu_exec_div(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_div_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    if (value2 != 0)
        cpu_exec_mov(value1, resolved_value1 / value2, size);
    else
        cpu_exec_mov(value1, 0, size);
}

static inline void cpu_exec_and(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_and_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 & value2, size);
}

static inline void cpu_exec_or(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_or_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 | value2, size);
}

static inline void cpu_exec_xor(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_reg)
    {
        cpu_xor_reg(value1, value2);
        return;
    }
    uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 ^ value2, size);
}

static inline uint8_t cpu_resolve_flags(uint8_t size)
{
    uint8_t flags;
    switch (size)
    {
    case 1:
        return cpu_reg_flagsl;
    case 2:
        return cpu_reg_flags;
    case 4:
        return cpu_reg_eflags;
    }
    return 0xff;
}

static inline void cpu_exec_cmp(uint64_t value1, uint64_t value2, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    if (value1 == value2)
        cpu_or_reg(flags, cpu_flags_ZF);
    else
        cpu_and_reg(flags, ~cpu_flags_ZF);
    if (value1 < value2)
        cpu_or_reg(flags, cpu_flags_CF);
    else
        cpu_and_reg(flags, ~cpu_flags_CF);
}

static inline void cpu_exec_jmp_near(uint8_t reg, uint64_t value1, uint8_t size)
{
    value1 = cpu_resolve_value(value1, 0, size);
    cpu_add_reg(reg, value1 - cpu_read_reg(reg));
}

static inline void cpu_exec_je(uint8_t reg, uint64_t value1, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    if (cpu_read_reg(flags) & cpu_flags_ZF)
        cpu_exec_jmp_near(reg, value1, size);
    else
        cpu_add_reg(reg, 1);
}

static inline void cpu_exec_jne(uint8_t reg, uint64_t value1, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    value1 = cpu_resolve_value(value1, 0, size);
    if (~cpu_read_reg(flags) & cpu_flags_ZF)
        cpu_exec_jmp_near(reg, value1, size);
    else
        cpu_add_reg(reg, 1);
}

static inline void cpu_exec_jc(uint8_t reg, uint64_t value1, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    value1 = cpu_resolve_value(value1, 0, size);
    if (cpu_read_reg(flags) & cpu_flags_CF)
        cpu_exec_jmp_near(reg, value1, size);
    else
        cpu_add_reg(reg, 1);
}

static inline void cpu_exec_jnc(uint8_t reg, uint64_t value1, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    value1 = cpu_resolve_value(value1, 0, size);
    if (~cpu_read_reg(flags) & cpu_flags_CF)
        cpu_exec_jmp_near(reg, value1, size);
    else
        cpu_add_reg(reg, 1);
}

static inline void cpu_exec_jz(uint8_t reg, uint64_t value1, uint8_t size)
{
    return cpu_exec_je(reg, value1, size);
}

static inline void cpu_exec_jnz(uint8_t reg, uint64_t value1, uint8_t size)
{
    return cpu_exec_jne(reg, value1, size);
}

static inline void cpu_push_reg(uint8_t stack, uint8_t reg, uint8_t size)
{
    cpu_write_reg(stack, cpu_read_reg(stack) - size);
    switch (size)
    {
    case 1:
        *((uint8_t *)&vm_memory[cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    case 2:
        *((uint16_t *)&vm_memory[cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    case 4:
        *((uint32_t *)&vm_memory[cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    default:
        *((uint64_t *)&vm_memory[cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    }
}

static inline void cpu_push_int(uint8_t stack, uint64_t value, uint8_t size)
{
    cpu_write_reg(stack, cpu_read_reg(stack) - size);
    switch (size)
    {
    case 1:
        *((uint8_t *)&vm_memory[cpu_read_reg(stack)]) = value;
        break;
    case 2:
        *((uint16_t *)&vm_memory[cpu_read_reg(stack)]) = value;
        break;
    case 4:
        *((uint32_t *)&vm_memory[cpu_read_reg(stack)]) = value;
        break;
    default:
        *((uint64_t *)&vm_memory[cpu_read_reg(stack)]) = value;
        break;
    }
}

static inline void cpu_pop_reg(uint8_t stack, uint8_t reg, uint8_t size)
{
    switch (size)
    {
    case 1:
        cpu_write_reg(reg, *((uint8_t *)&vm_memory[cpu_read_reg(stack)]));
        break;
    case 2:
        cpu_write_reg(reg, *((uint16_t *)&vm_memory[cpu_read_reg(stack)]));
        break;
    case 4:
        cpu_write_reg(reg, *((uint32_t *)&vm_memory[cpu_read_reg(stack)]));
        break;
    default:
        cpu_write_reg(reg, *((uint64_t *)&vm_memory[cpu_read_reg(stack)]));
        break;
    }
    cpu_write_reg(stack, cpu_read_reg(stack) + size);
}

void cpu_setup_precalcs()
{
    memset(cpu_state, 0, sizeof(cpu_state));
}

void cpu_dump_state()
{
    printf("cpu debug : {\n");
    for (size_t i = 0; i < cpu_reg_end; i++)
        if (cpu_regs_string[i])
            printf("%s: 0x%lx,", cpu_regs_string[i], cpu_read_reg(i));
    printf("\n};\n");
}

static inline void cpu_rm_resolve(uint8_t rm8, uint8_t size)
{
    switch (size)
    {
    case 1 ... 2:
        switch (rm8)
        {
        case 0x00:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_si;
            break;
        case 0x01:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_di;
            break;
        case 0x02:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_si;
            break;
        case 0x03:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_di;
            break;
        case 0x04:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_si;
            break;
        case 0x05:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_di;
            break;
        case 0x06:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp;
            break;
        case 0x07:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx;
            break;
        }
        break;
    case 0x04:
        switch (rm8)
        {
        case 0x00:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_si;
            break;
        case 0x01:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_di;
            break;
        case 0x02:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_si;
            break;
        case 0x03:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_di;
            break;
        case 0x04:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_esp;
            break;
        case 0x05:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebp;
            break;
        case 0x06:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp;
            break;
        case 0x07:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx;
            break;
        }
        break;
    default:
        break;
    }
}

static inline uint64_t cpu_rm(uint8_t reg, uint8_t size, uint8_t override_size)
{
    uint8_t mod = (opcode[1] & 0xc0) >> 0x06;
    uint8_t rm8 = opcode[1] & 0x07;
    uint64_t value = 0;
    if (opcode[2] == 0x24)
        opcode++, cpu_add_reg(reg, 1);
    switch (mod)
    {
    case 0x00:
        if (rm8 == 0x06)
        {
            switch (override_size)
            {
            case 1 ... 2:
                value = (int64_t)(0xffff & *(int16_t *)&opcode[2]);
                override_size = 2;
                break;
            case 4:
                value = (int64_t)(0xffffffff & *(int32_t *)&opcode[2]);
                break;
            default:
                break;
            }
            opcode += override_size, cpu_add_reg(reg, override_size);
            cpu_info[cpu_info_index].reg_type = cpu_type_memory;
            goto ret;
        }
        break;
    case 0x01:
        value = (int64_t)(0xff & *(int8_t *)&opcode[2]);
        cpu_info[cpu_info_index].reg_type = cpu_type_memory;
        opcode++, cpu_add_reg(reg, 1);
        break;
    case 0x02:
        switch (override_size)
        {
        case 1 ... 2:
            value = (int64_t)(0xffff & *(int16_t *)&opcode[2]);
            override_size = 2;
            break;
        case 4:
            value = (int64_t)(0xffffffff & *(int32_t *)&opcode[2]);
            break;
        default:
            break;
        }
        opcode += override_size, cpu_add_reg(reg, override_size);
        cpu_info[cpu_info_index].reg_type = cpu_type_memory;
        break;
    case 0x03:
        switch (size)
        {
        case 1:
            value = regs8[rm8];
            cpu_info[cpu_info_index].reg_type = cpu_type_reg;
            break;
        case 2:
            value = regs16[rm8];
            cpu_info[cpu_info_index].reg_type = cpu_type_reg;
            break;
        case 4:
            value = regs32[rm8];
            cpu_info[cpu_info_index].reg_type = cpu_type_reg;
            break;
        }
        goto ret;
    }
    cpu_rm_resolve(rm8, size);
ret:
    opcode++, cpu_add_reg(reg, 1), cpu_info_index++;
    return value;
}

static inline uint64_t cpu_r_rm(uint8_t reg, void *r, uint8_t r_size, uint8_t size, uint8_t override_size)
{
    uint8_t *opcode_backup = opcode;
    switch (r_size)
    {
    case 1:
        *(uint64_t *)r = regs8[(opcode_backup[1] & 0x38) >> 3];
        break;
    case 2:
        *(uint64_t *)r = regs16[(opcode_backup[1] & 0x38) >> 3];
        break;
    case 4:
        *(uint64_t *)r = regs32[(opcode_backup[1] & 0x38) >> 3];
        break;
    case 255:
        *(uint64_t *)r = segregs[(opcode_backup[1] & 0x38) >> 3];
        break;
    default:
        break;
    }
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    uint64_t rm = cpu_rm(reg, size, override_size);
    return rm;
}

static inline uint64_t cpu_rm_r(uint8_t reg, void *r, uint8_t r_size, uint8_t size, uint8_t override_size)
{
    uint8_t *opcode_backup = opcode;
    switch (r_size)
    {
    case 1:
        *(uint64_t *)r = regs8[(opcode_backup[1] & 0x38) >> 3];
        break;
    case 2:
        *(uint64_t *)r = regs16[(opcode_backup[1] & 0x38) >> 3];
        break;
    case 4:
        *(uint64_t *)r = regs32[(opcode_backup[1] & 0x38) >> 3];
        break;
    case 255:
        *(uint64_t *)r = segregs[(opcode_backup[1] & 0x38) >> 3];
        break;
    default:
        break;
    }
    uint64_t rm = cpu_rm(reg, size, override_size);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return rm;
}

static inline uint8_t cpu_get_segment_override(uint16_t override)
{
    if (override & cpu_override_es)
        return cpu_reg_es;
    if (override & cpu_override_cs)
        return cpu_reg_cs;
    if (override & cpu_override_ss)
        return cpu_reg_ss;
    if (override & cpu_override_ds)
        return cpu_reg_ds;
    return 0xff;
}

static inline uint64_t cpu_m(uint8_t reg, uint16_t override, uint8_t size)
{
    uint8_t sreg = cpu_get_segment_override(override);
    if (sreg == 0xff)
        goto error;
    cpu_info[cpu_info_index].segmentation = sreg;
    cpu_info[cpu_info_index].reg_type = cpu_type_memory;
    switch (size)
    {
    case 1:
        return *(uint8_t *)&opcode[1];
    case 2:
        return *(uint16_t *)&opcode[1];
    case 4:
        return *(uint32_t *)&opcode[1];
    }
error:
    return (uint64_t)-1;
}

static inline uint16_t cpu_m16(uint8_t reg, uint16_t override)
{
    return (uint16_t)cpu_m(reg, override, 2);
}

static inline uint8_t cpu_r32(uint8_t reg)
{
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return regs32[(*opcode & 0x38) & 7];
}

static inline uint8_t cpu_r16(uint8_t reg)
{
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return regs16[(*opcode & 0x38) & 7];
}

static inline uint8_t cpu_r8(uint8_t reg)
{
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return regs8[(*opcode & 0x38) & 7];
}

static inline uint16_t cpu_rm8_r8(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_rm_r(reg, r, 1, 1, override_size);
}

static inline uint16_t cpu_rm16_r16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_rm_r(reg, r, 2, 2, override_size);
}

static inline uint16_t cpu_rm16_r8(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_rm_r(reg, r, 1, 2, override_size);
}

static inline uint16_t cpu_rm16_sr(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_rm_r(reg, r, 255, 2, override_size);
}

static inline uint16_t cpu_rm8_r16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_rm_r(reg, r, 2, 1, override_size);
}

static inline uint16_t cpu_r8_rm8(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_r_rm(reg, r, 1, 1, override_size);
}

static inline uint16_t cpu_r16_rm16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_r_rm(reg, r, 2, 2, override_size);
}

static inline uint16_t cpu_r8_rm16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_r_rm(reg, r, 1, 2, override_size);
}

static inline uint16_t cpu_r16_rm8(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_r_rm(reg, r, 2, 1, override_size);
}

static inline uint16_t cpu_sr_rm16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint16_t)cpu_r_rm(reg, r, 255, 2, override_size);
}

static inline uint32_t cpu_rm32_r32(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_rm_r(reg, r, 4, 4, override_size);
}

static inline uint32_t cpu_r32_rm32(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_r_rm(reg, r, 4, 4, override_size);
}

static inline uint32_t cpu_rm32_r8(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_rm_r(reg, r, 1, 4, override_size);
}

static inline uint32_t cpu_r8_rm32(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_r_rm(reg, r, 1, 4, override_size);
}

static inline uint32_t cpu_rm32_r16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_rm_r(reg, r, 2, 4, override_size);
}

static inline uint32_t cpu_rm16_r32(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_rm_r(reg, r, 4, 2, override_size);
}

static inline uint32_t cpu_r16_rm32(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_r_rm(reg, r, 2, 4, override_size);
}

static inline uint16_t cpu_rm32(uint8_t reg, uint8_t override_size)
{
    return (uint16_t)cpu_rm(reg, 4, override_size);
}

static inline uint16_t cpu_rm16(uint8_t reg, uint8_t override_size)
{
    return (uint16_t)cpu_rm(reg, 2, override_size);
}

static inline uint8_t cpu_rm8(uint8_t reg, uint8_t override_size)
{
    return (uint8_t)cpu_rm(reg, 1, override_size);
}

static inline uint32_t cpu_imm32(uint8_t reg)
{
    uint32_t imm = *(uint32_t *)&opcode[1];
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 4);
    return (uint32_t)imm;
}

static inline uint16_t cpu_imm16(uint8_t reg)
{
    uint16_t imm = *(uint16_t *)&opcode[1];
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 2);
    return (uint16_t)imm;
}

static inline uint8_t cpu_imm8(uint8_t reg)
{
    uint8_t imm = opcode[1];
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    return (uint8_t)imm;
}

static inline uint32_t cpu_rel32(uint8_t reg)
{
    uint32_t rel = *(uint32_t *)&opcode[1];
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 4);
    return (uint32_t)(cpu_read_reg(reg) + rel + 1);
}

static inline uint16_t cpu_rel16(uint8_t reg)
{
    uint16_t rel = *(uint16_t *)&opcode[1];
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 2);
    return (uint16_t)(cpu_read_reg(reg) + rel + 1);
}

static inline uint8_t cpu_rel8(uint8_t reg)
{
    uint8_t rel = opcode[1];
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    return (uint8_t)(cpu_read_reg(reg) + rel + 1);
}

static inline uint8_t cpu_sr(uint8_t reg)
{
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    return segregs[(opcode[1] & 0x38) >> 3];
}

static inline void cpu_print_instruction(char *instruction, char *type,
                                         uint8_t regs, ...)
{
    int16_t print_pos = -1;
    va_list args;
    va_start(args, regs);
    if (regs)
    {
        printf("%s ", instruction);
        if (type)
        {
            for (size_t i = 0; type[i] != '\0'; i++)
            {
                if (type[i] >= '0' && type[i] <= '9')
                {
                    if (print_pos < 0)
                        print_pos = 0;
                    if (print_pos * 10 + type[i] - '0' > 0xff)
                        break;
                    print_pos *= 10;
                    print_pos += type[i] - '0';
                    type++;
                }
            }
            if (print_pos < 0)
                printf("%s ", type);
        }
    }
    else
    {
        printf("%s", instruction);
    }
    for (uint8_t index = 0; index < regs; index++)
    {
        uint64_t value = va_arg(args, uint64_t);
        if (index == print_pos)
            printf("%s ", type);
        switch (cpu_info[index].reg_type)
        {
        case cpu_type_reg:
            printf("%s", cpu_regs_string[value]);
            break;
        case cpu_type_int:
            printf("0x%lx", value);
            break;
        case cpu_type_memory:
            printf("[0x%lx]", value);
            break;
        case cpu_type_memory_reg:
            if (cpu_info[index].reg_type_buffer[0])
                cpu_info[index].reg_type_buffer[0] == 1 ? printf("[%s + 0x%lx]",
                                                                 cpu_regs_string[cpu_info[index].reg_type_buffer[1]],
                                                                 value)
                                                        : printf("[%s + %s + 0x%lx]",
                                                                 cpu_regs_string[cpu_info[index].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[index].reg_type_buffer[2]],
                                                                 value);
            else
                printf("[%s]", cpu_regs_string[value]);
            break;
        default:
            break;
        }
        if (regs - index > 1)
            printf(", ");
        else
            break;
    }
    printf("\n");
}

void cpu_emulate_i8086(uint8_t debug, uint8_t override)
{
    gdt_entry_t *entry = (gdt_entry_t *)(&vm_memory[((gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)])->base] +
                                         cpu_read_reg(cpu_reg_cs));
    uint64_t base = (entry->base_high << 24) |
                    (entry->base_middle << 16) |
                    entry->base_low;
    opcode = &vm_memory[base + cpu_read_reg(cpu_reg_ip)];
    uint64_t value1;
    uint64_t value2;
    char *operation;
    switch (*opcode)
    {
    case 0x66 ... 0x67:
        break;
    case 0x64 ... 0x65:
        break;
    case 0x26:
        break;
    case 0x3e:
        break;
    case 0x2e:
        break;
    case 0x36:
        break;
    default:
        if (debug)
            printf("OPCODE: 0x%x, ESP: 0x%lx; ", *opcode, cpu_read_reg(cpu_reg_esp));
        break;
    }
    switch (*opcode)
    {
    case 0x06:
        cpu_push_reg(cpu_reg_sp, cpu_reg_es, 2);
        if (debug)
            printf("push word es\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x07:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_es, 2);
        if (debug)
            printf("pop word es\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x0e:
        cpu_push_reg(cpu_reg_sp, cpu_reg_cs, 2);
        if (debug)
            printf("push word cs\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x0f:
        opcode++, cpu_add_reg(cpu_reg_ip, 1);
        uint8_t error = 0, size = 0;
        switch ((opcode[0] & 0x38) >> 3)
        {
        case 0x00:
            switch ((opcode[1] >> 2) & 0x07)
            {
            case 0x01:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jnc(cpu_reg_ip, value1, 2);
                if (debug)
                    printf("jnc 0x%lx\n", value1);
                break;
            case 0x02:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jc(cpu_reg_ip, value1, 2);
                if (debug)
                    printf("jc 0x%lx\n", value1);
                break;
            case 0x03:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jnz(cpu_reg_ip, value1, 2);
                if (debug)
                    printf("jnz 0x%lx\n", value1);
                break;
            case 0x04:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jz(cpu_reg_ip, value1, 2);
                if (debug)
                    printf("jz 0x%lx\n", value1);
                break;
            case 0x05:
                switch ((opcode[1] & 0x38) >> 3)
                {
                case 0x02:
                    value1 = cpu_rm16(cpu_reg_ip, 2);
                    value2 = cpu_resolve_value(value1, 0, 2);
                    cpu_write_reg(cpu_reg_gdtr_next, value2);
                    if (debug)
                        cpu_print_instruction("lgdt", (void *)NULL, 1, value1, (uint64_t)0);
                    cpu_add_reg(cpu_reg_ip, 1);
                    break;
                default:
                    value1 = cpu_rel16(cpu_reg_ip);
                    cpu_exec_jne(cpu_reg_ip, value1, 2);
                    if (debug)
                        printf("jne 0x%lx\n", value1);
                    break;
                }
                break;
            case 0x06:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_je(cpu_reg_ip, value1, 2);
                if (debug)
                    printf("je 0x%lx\n", value1);
                break;
            default:
                error = (opcode[1] >> 2) & 0x07;
                goto is_pop_cs;
            }
            break;
        case 0x06:
            switch (opcode[0] & 0x07)
            {
            case 0x06:
                size = 1;
                break;
            case 0x07:
                size = 2;
                break;
            }
            if (override & cpu_override_dword_operand)
            {
                if (override & cpu_override_dword_address)
                    value2 = cpu_r32_rm32(cpu_reg_ip, &value1, 4);
                else
                    value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
            }
            else
            {
                value2 = cpu_r16_rm16(cpu_reg_ip, &value1, 2);
            }
            cpu_exec_mov(value1, value2, size);
            switch (size)
            {
            case 1:
                if (debug)
                    cpu_print_instruction("movzx", "1byte", 2, value1, value2);
                break;
            case 2:
                if (debug)
                    cpu_print_instruction("movzx", "1word", 2, value1, value2);
                break;
            }
            cpu_add_reg(cpu_reg_ip, 1);
            break;
        is_pop_cs:
        default:
            cpu_pop_reg(cpu_reg_sp, cpu_reg_cs, 2);
            if (debug)
                printf("pop word cs; %x, %x\n", (opcode[0] & 0x38) >> 3, (opcode[1] & 0x38) >> 3);
            cpu_add_reg(cpu_reg_ip, 1);
            break;
        }
        break;
    case 0x16:
        cpu_push_reg(cpu_reg_sp, cpu_reg_ss, 2);
        if (debug)
            printf("push word ss\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x17:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_ss, 2);
        if (debug)
            printf("pop word ss\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x1e:
        cpu_push_reg(cpu_reg_sp, cpu_reg_ds, 2);
        if (debug)
            printf("push word ds\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x1f:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_ds, 2);
        if (debug)
            printf("pop word ds\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x26:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_es);
        break;
    case 0x2e:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_cs);
        break;
    case 0x30:
        value1 = cpu_rm8(cpu_reg_ip, 1);
        value2 = cpu_r8(cpu_reg_ip);
        cpu_exec_xor(value1, value2, 1);
        if (debug)
            cpu_print_instruction("xor", "byte", 2, value1, value2);
        break;
    case 0x31:
        value1 = cpu_rm16(cpu_reg_ip, 2);
        value2 = cpu_r16(cpu_reg_ip);
        cpu_exec_xor(value1, value2, 2);
        if (debug)
            cpu_print_instruction("xor", "word", 2, value1, value2);
        break;
    case 0x32:
        value1 = cpu_r8(cpu_reg_ip);
        value2 = cpu_rm8(cpu_reg_ip, 1);
        cpu_exec_xor(value1, value2, 1);
        if (debug)
            cpu_print_instruction("xor", "byte", 2, value1, value2);
        break;
    case 0x33:
        value1 = cpu_r16(cpu_reg_ip);
        value2 = cpu_rm16(cpu_reg_ip, 2);
        cpu_exec_xor(value1, value2, 2);
        if (debug)
            cpu_print_instruction("xor", "word", 2, value1, value2);
        break;
    case 0x36:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_ss);
        break;
    case 0x38:
        value1 = cpu_rm8_r8(cpu_reg_ip, &value2, 1);
        cpu_exec_cmp(value1, value2, 1);
        if (debug)
            cpu_print_instruction("cmp", "byte", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x39:
        value1 = cpu_rm16_r16(cpu_reg_ip, &value2, 2);
        cpu_exec_cmp(value1, value2, 2);
        if (debug)
            cpu_print_instruction("cmp", "word", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x3a:
        value2 = cpu_r8_rm8(cpu_reg_ip, &value1, 1);
        cpu_exec_cmp(value1, value2, 1);
        if (debug)
            cpu_print_instruction("cmp", "byte", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x3b:
        value2 = cpu_r16_rm16(cpu_reg_ip, &value1, 2);
        cpu_exec_cmp(value1, value2, 2);
        if (debug)
            cpu_print_instruction("cmp", "word", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x3e:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_ds);
        break;
    case 0x50:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_eax, 4);
            if (debug)
                printf("push dword eax\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_ax, 2);
            if (debug)
                printf("push word ax\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x51:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_ecx, 4);
            if (debug)
                printf("push dword ecx\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_cx, 2);
            if (debug)
                printf("push word cx\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x52:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_edx, 4);
            if (debug)
                printf("push dword edx\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_dx, 2);
            if (debug)
                printf("push word dx\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x53:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_ebx, 4);
            if (debug)
                printf("push dword ebx\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_bx, 2);
            if (debug)
                printf("push word bx\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x54:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_esp, 4);
            if (debug)
                printf("push dword esp\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_sp, 2);
            if (debug)
                printf("push word sp\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x55:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_ebp, 4);
            if (debug)
                printf("push dword ebp\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_bp, 2);
            if (debug)
                printf("push word bp\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x56:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_esi, 4);
            if (debug)
                printf("push dword esi\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_si, 2);
            if (debug)
                printf("push word si\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x57:
        if (override & cpu_override_dword_operand)
        {
            cpu_push_reg(cpu_reg_esp, cpu_reg_edi, 4);
            if (debug)
                printf("push dword edi\n");
        }
        else
        {
            cpu_push_reg(cpu_reg_sp, cpu_reg_di, 2);
            if (debug)
                printf("push word di\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x58:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_eax, 4);
            if (debug)
                printf("pop dword eax\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_ax, 2);
            if (debug)
                printf("pop word ax\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x59:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_ecx, 4);
            if (debug)
                printf("pop dword ecx\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_cx, 2);
            if (debug)
                printf("pop word cx\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x5a:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_edx, 4);
            if (debug)
                printf("pop dword edx\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_dx, 2);
            if (debug)
                printf("pop word dx\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x5b:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_ebx, 4);
            if (debug)
                printf("pop dword ebx\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_bx, 2);
            if (debug)
                printf("pop word bx\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x5c:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_esp, 4);
            if (debug)
                printf("pop dword esp\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_sp, 2);
            if (debug)
                printf("pop word sp\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x5d:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_ebp, 4);
            if (debug)
                printf("pop dword ebp\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_bp, 2);
            if (debug)
                printf("pop word bp\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x5e:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_esi, 4);
            if (debug)
                printf("pop dword esi\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_si, 2);
            if (debug)
                printf("pop word si\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x5f:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_esp, cpu_reg_edi, 4);
            if (debug)
                printf("pop dword edi\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_di, 2);
            if (debug)
                printf("pop word di\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x60:
        if (override & cpu_override_dword_operand)
        {
            for (uint8_t i = cpu_reg_eax; i < cpu_reg_eflags + 1; i += 2)
                cpu_push_reg(cpu_reg_esp, i, 4);
            if (debug)
                printf("pushad\n");
        }
        else
        {
            for (uint8_t i = cpu_reg_ax; i < cpu_reg_flags + 1; i += 2)
                cpu_push_reg(cpu_reg_sp, i, 2);
            if (debug)
                printf("pusha\n");
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x61:
        value1 = cpu_read_reg(cpu_reg_ip);
        if (override & cpu_override_dword_operand)
        {
            for (uint8_t i = cpu_reg_eax; i < cpu_reg_eflags + 1; i += 2)
                cpu_pop_reg(cpu_reg_esp, cpu_reg_flags - i, 4);
            if (debug)
                printf("popad\n");
        }
        else
        {
            for (uint8_t i = cpu_reg_ax; i < cpu_reg_flags + 1; i += 2)
                cpu_pop_reg(cpu_reg_sp, cpu_reg_flags - i, 2);
            if (debug)
                printf("popa\n");
        }
        cpu_write_reg(cpu_reg_ip, value1 + 1);
        break;
    case 0x64:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_fs);
        break;
    case 0x65:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_gs);
        break;
    case 0x66:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_dword_operand);
        break;
    case 0x67:
        cpu_add_reg(cpu_reg_ip, 1);
        cpu_emulate_i8086(debug, override | cpu_override_dword_address);
        break;
    case 0x6a:
        value2 = cpu_imm8(cpu_reg_ip);
        cpu_push_int(cpu_reg_sp, value2, 2);
        if (debug)
            printf("push byte 0x%lx\n", value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x68:
        value2 = cpu_imm16(cpu_reg_ip);
        cpu_push_int(cpu_reg_sp, value2, 2);
        if (debug)
            printf("push word 0x%lx\n", value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x74:
        value1 = cpu_rel8(cpu_reg_ip);
        cpu_exec_je(cpu_reg_ip, value1, 2);
        if (debug)
            printf("je 0x%lx\n", value1);
        break;
    case 0x80:
        switch ((opcode[1] & 0x38) >> 3)
        {
        case 0x00:
            value1 = cpu_rm8(cpu_reg_ip, 1);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_add(value1, value2, 1);
            if (debug)
                cpu_print_instruction("add", "byte", 2, value1, value2);
            break;
        case 0x01:
            value1 = cpu_rm8(cpu_reg_ip, 1);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_or(value1, value2, 1);
            if (debug)
                cpu_print_instruction("or", "byte", 2, value1, value2);
            break;
        case 0x04:
            value1 = cpu_rm8(cpu_reg_ip, 1);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_and(value1, value2, 1);
            if (debug)
                cpu_print_instruction("and", "byte", 2, value1, value2);
            break;
        case 0x05:
            value1 = cpu_rm8(cpu_reg_ip, 1);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_sub(value1, value2, 1);
            if (debug)
                cpu_print_instruction("sub", "byte", 2, value1, value2);
            break;
        case 0x06:
            value1 = cpu_rm8(cpu_reg_ip, 1);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_xor(value1, value2, 1);
            if (debug)
                cpu_print_instruction("xor", "byte", 2, value1, value2);
            break;
        case 0x07:
            value1 = cpu_rm8(cpu_reg_ip, 1);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_cmp(value1, value2, 1);
            if (debug)
                cpu_print_instruction("cmp", "byte", 2, value1, value2);
            break;
        default:
            goto unknown;
            break;
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x81:
        switch ((opcode[1] & 0x38) >> 3)
        {
        case 0x00:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_add(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("add", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_add(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("add", "word", 2, value1, value2);
            }
            break;
        case 0x01:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_or(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("or", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_or(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("or", "word", 2, value1, value2);
            }
            break;
        case 0x04:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_and(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("and", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_and(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("and", "word", 2, value1, value2);
            }
            break;
        case 0x05:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_sub(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("sub", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_sub(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("sub", "word", 2, value1, value2);
            }
            break;
        case 0x06:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_xor(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("xor", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_xor(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("xor", "word", 2, value1, value2);
            }
            break;
        case 0x07:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_cmp(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("cmp", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_cmp(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("cmp", "word", 2, value1, value2);
            }
            break;
        default:
            goto unknown;
            break;
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x83:
        switch ((opcode[1] & 0x38) >> 3)
        {
        case 0x00:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_add(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("add", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_add(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("add", "word", 2, value1, value2);
            }
            break;
        case 0x01:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_or(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("or", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_or(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("or", "word", 2, value1, value2);
            }
            break;
        case 0x04:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_and(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("and", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_and(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("and", "word", 2, value1, value2);
            }
            break;
        case 0x05:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_sub(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("sub", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_sub(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("sub", "word", 2, value1, value2);
            }
            break;
        case 0x06:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_xor(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("xor", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_xor(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("xor", "word", 2, value1, value2);
            }
            break;
        case 0x07:
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_cmp(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("cmp", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16(cpu_reg_ip, 2);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_cmp(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("cmp", "word", 2, value1, value2);
            }
            break;
        default:
            goto unknown;
            break;
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x88:
        if (override & cpu_override_dword_address)
            value1 = cpu_rm32_r8(cpu_reg_ip, &value2, 4);
        else
            value1 = cpu_rm8_r8(cpu_reg_ip, &value2, 1);
        cpu_exec_mov(value1, value2, 1);
        if (debug)
            cpu_print_instruction("mov", "byte", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x89:
        if (override & cpu_override_dword_operand)
        {
            value1 = cpu_rm32_r32(cpu_reg_ip, &value2, 4);
            cpu_exec_mov(value1, value2, 4);
            if (debug)
                cpu_print_instruction("mov", "dword", 2, value1, value2);
        }
        else
        {
            value1 = cpu_rm16_r16(cpu_reg_ip, &value2, 2);
            cpu_exec_mov(value1, value2, 2);
            if (debug)
                cpu_print_instruction("mov", "word", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x8c:
        value1 = cpu_rm16_sr(cpu_reg_ip, &value2, 2);
        cpu_exec_mov(value1, value2, 2);
        if (debug)
            cpu_print_instruction("mov", "word", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x8e:
        value2 = cpu_sr_rm16(cpu_reg_ip, &value1, 2);
        cpu_exec_mov(value1, value2, 2);
        if (debug)
            cpu_print_instruction("mov", "word", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x8a:
        if (override & cpu_override_dword_address)
            value2 = cpu_r8_rm32(cpu_reg_ip, &value1, 4);
        else
            value2 = cpu_r8_rm16(cpu_reg_ip, &value1, 1);
        cpu_exec_mov(value1, value2, 1);
        if (debug)
            cpu_print_instruction("mov", "byte", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x8b:
        if (override & cpu_override_dword_operand)
        {
            value2 = cpu_r32_rm32(cpu_reg_ip, &value1, 4);
            cpu_exec_mov(value1, value2, 4);
            if (debug)
                cpu_print_instruction("mov", "dword", 2, value1, value2);
        }
        else
        {
            if (override & cpu_override_dword_address)
                value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
            else
                value2 = cpu_r16_rm16(cpu_reg_ip, &value1, 2);
            cpu_exec_mov(value1, value2, 2);
            if (debug)
                cpu_print_instruction("mov", "word", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x90:
        if (debug)
            printf("nop\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb0:
        cpu_write_reg(cpu_reg_al, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte al, 0x%lx\n", cpu_read_reg(cpu_reg_al));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb1:
        cpu_write_reg(cpu_reg_cl, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte cl, 0x%lx\n", cpu_read_reg(cpu_reg_cl));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb2:
        cpu_write_reg(cpu_reg_dl, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte dl, 0x%lx\n", cpu_read_reg(cpu_reg_dl));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb3:
        cpu_write_reg(cpu_reg_bl, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte bl, 0x%lx\n", cpu_read_reg(cpu_reg_bl));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb4:
        cpu_write_reg(cpu_reg_ah, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte ah, 0x%lx\n", cpu_read_reg(cpu_reg_ah));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb5:
        cpu_write_reg(cpu_reg_ch, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte ch, 0x%lx\n", cpu_read_reg(cpu_reg_ch));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb6:
        cpu_write_reg(cpu_reg_dh, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte dh, 0x%lx\n", cpu_read_reg(cpu_reg_dh));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb7:
        cpu_write_reg(cpu_reg_bh, cpu_imm8(cpu_reg_ip));
        if (debug)
            printf("mov byte bh, 0x%lx\n", cpu_read_reg(cpu_reg_bh));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb8:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_eax, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword eax, 0x%lx\n", cpu_read_reg(cpu_reg_eax));
        }
        else
        {
            cpu_write_reg(cpu_reg_ax, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word ax, 0x%lx\n", cpu_read_reg(cpu_reg_ax));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb9:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_ecx, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword ecx, 0x%lx\n", cpu_read_reg(cpu_reg_ecx));
        }
        else
        {
            cpu_write_reg(cpu_reg_cx, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word cx, 0x%lx\n", cpu_read_reg(cpu_reg_cx));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xba:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_edx, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword edx, 0x%lx\n", cpu_read_reg(cpu_reg_edx));
        }
        else
        {
            cpu_write_reg(cpu_reg_dx, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word dx, 0x%lx\n", cpu_read_reg(cpu_reg_dx));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xbb:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_ebx, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword ebx, 0x%lx\n", cpu_read_reg(cpu_reg_ebx));
        }
        else
        {
            cpu_write_reg(cpu_reg_bx, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word bx, 0x%lx\n", cpu_read_reg(cpu_reg_bx));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xbc:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_esp, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword esp, 0x%lx\n", cpu_read_reg(cpu_reg_esp));
        }
        else
        {
            cpu_write_reg(cpu_reg_sp, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word sp, 0x%lx\n", cpu_read_reg(cpu_reg_sp));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xbd:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_ebp, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword ebp, 0x%lx\n", cpu_read_reg(cpu_reg_ebp));
        }
        else
        {
            cpu_write_reg(cpu_reg_bp, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word bp, 0x%lx\n", cpu_read_reg(cpu_reg_bp));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xbe:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_esi, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword esi, 0x%lx\n", cpu_read_reg(cpu_reg_esi));
        }
        else
        {
            cpu_write_reg(cpu_reg_si, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word si, 0x%lx\n", cpu_read_reg(cpu_reg_si));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xbf:
        if (override & cpu_override_dword_operand)
        {
            cpu_write_reg(cpu_reg_edi, cpu_imm32(cpu_reg_ip));
            if (debug)
                printf("mov dword edi, 0x%lx\n", cpu_read_reg(cpu_reg_edi));
        }
        else
        {
            cpu_write_reg(cpu_reg_di, cpu_imm16(cpu_reg_ip));
            if (debug)
                printf("mov word di, 0x%lx\n", cpu_read_reg(cpu_reg_di));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xc3:
        if (override & cpu_override_dword_operand)
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_ip, 4);
            if (debug)
                printf("retd\n");
        }
        else
        {
            cpu_pop_reg(cpu_reg_sp, cpu_reg_ip, 2);
            if (debug)
                printf("ret\n");
        }
        break;
    case 0xc6:
        if (override & cpu_override_dword_address)
        {
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_r32(cpu_reg_ip);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_mov(value1, value2, 1);
                if (debug)
                    cpu_print_instruction("mov", "byte", 2, value1, value2);
            }
            else
            {
                value1 = cpu_r32(cpu_reg_ip);
                value2 = cpu_imm8(cpu_reg_ip);
                cpu_exec_mov(value1, value2, 1);
                if (debug)
                    cpu_print_instruction("mov", "byte", 2, value1, value2);
            }
        }
        else
        {
            value1 = cpu_r16(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            cpu_exec_mov(value1, value2, 1);
            if (debug)
                cpu_print_instruction("mov", "byte", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xc7:
        if (override & cpu_override_dword_operand)
        {
            if (override & cpu_override_dword_address)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip);
                cpu_exec_mov(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("mov", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm16(cpu_reg_ip);
                cpu_exec_mov(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("mov", "word", 2, value1, value2);
            }
        }
        else
        {
            value1 = cpu_rm16(cpu_reg_ip, 2);
            value2 = cpu_imm16(cpu_reg_ip);
            cpu_exec_mov(value1, value2, 2);
            if (debug)
                cpu_print_instruction("mov", "word", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xcd:
        value2 = cpu_imm8(cpu_reg_ip);
        uint16_t ax, bx, cx, dx;
        switch (value2)
        {
        case 0x20:
            ax = cpu_read_reg(cpu_reg_ax),
            bx = cpu_read_reg(cpu_reg_bx),
            cx = cpu_read_reg(cpu_reg_cx),
            dx = cpu_read_reg(cpu_reg_dx);
            *(uint32_t *)&vm_memory[window_framebuffer[0] + (cx * window_framebuffer[1] + dx) * sizeof(uint32_t)] = (bx << 16) | ax;
            break;
        case 0x21:
            memory_write(0xfff0, window_framebuffer[1], 2);
            memory_write(0xfff2, window_framebuffer[2], 2);
            memory_write(0xfff4, window_framebuffer[3], 2);
            break;
        default:
            break;
        }
        if (debug)
            printf("int byte 0x%llx\n", (unsigned long long)value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe4:
        value1 = cpu_imm8(cpu_reg_ip);
        cpu_write_reg(cpu_reg_al, io_read(value1, 1));
        if (debug)
            cpu_print_instruction("in al,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe5:
        value1 = cpu_imm8(cpu_reg_ip);
        cpu_write_reg(cpu_reg_ax, io_read(value1, 2));
        if (debug)
            cpu_print_instruction("in ax,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe6:
        value1 = cpu_imm8(cpu_reg_ip);
        io_write(cpu_read_reg(cpu_reg_al), value1, 1);
        if (debug)
            cpu_print_instruction("out al,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe7:
        value1 = cpu_imm8(cpu_reg_ip);
        io_write(cpu_read_reg(cpu_reg_ax), value1, 2);
        if (debug)
            cpu_print_instruction("out ax,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe8:
        if (override & cpu_override_dword_operand)
        {
            value1 = cpu_rel32(cpu_reg_ip);
            cpu_add_reg(cpu_reg_ip, 1);
            cpu_push_reg(cpu_reg_sp, cpu_reg_ip, 4);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("call dword 0x%lx\n", value1);
        }
        else
        {
            value1 = cpu_rel16(cpu_reg_ip);
            cpu_add_reg(cpu_reg_ip, 1);
            cpu_push_reg(cpu_reg_sp, cpu_reg_ip, 2);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("call word 0x%lx\n", value1);
        }
        break;
    case 0xe9:
        if (override & cpu_override_dword_operand)
        {
            value1 = cpu_rel32(cpu_reg_ip);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("jmp dword 0x%lx\n", value1);
        }
        else
        {
            value1 = cpu_rel16(cpu_reg_ip);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("jmp word 0x%lx\n", value1);
        }
        break;
    case 0xea:
        value1 = cpu_imm16(cpu_reg_ip);
        value2 = cpu_imm16(cpu_reg_ip);
        cpu_write_reg(cpu_reg_gdtr, cpu_read_reg(cpu_reg_gdtr_next));
        cpu_write_reg(cpu_reg_ip, value1);
        cpu_write_reg(cpu_reg_cs, value2);
        if (debug)
            printf("jmp 0x%lx:0x%lx\n", value2, value1);
        break;
    case 0xeb:
        value1 = cpu_rel8(cpu_reg_ip);
        cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
        if (debug)
            printf("jmp 0x%lx\n", value1);
        break;
    case 0xec:
        if (debug)
            printf("in al, dx\n");
        cpu_write_reg(cpu_reg_dx, io_read(cpu_read_reg(cpu_reg_al), 1));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xed:
        if (debug)
            printf("in ax, dx\n");
        cpu_write_reg(cpu_reg_dx, io_read(cpu_read_reg(cpu_reg_ax), 2));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xee:
        if (debug)
            printf("out dx, al\n");
        io_write(cpu_read_reg(cpu_reg_dx), cpu_read_reg(cpu_reg_al), 1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xef:
        if (debug)
            printf("out dx, ax\n");
        io_write(cpu_read_reg(cpu_reg_dx), cpu_read_reg(cpu_reg_ax), 2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf0:
        if (debug)
            printf("lock\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf2:
        if (debug)
            printf("repne\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf3:
        if (debug)
            printf("rep\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf4:
        if (debug)
            printf("hlt\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf5:
        if (debug)
            printf("cmc\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf8:
        if (debug)
            printf("clc\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xf9:
        if (debug)
            printf("stc\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xfa:
        if (debug)
            printf("cli\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xfb:
        if (debug)
            printf("sti\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xfc:
        if (debug)
            printf("cld\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xfd:
        if (debug)
            printf("std\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    unknown:
    default:
        if (debug)
            printf("db 0x%hhx\n", *opcode);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    }
    cpu_info_index = 0;
}
