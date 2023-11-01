#include "cpu.h"

extern global_uint64_t window_framebuffer[];

cpu_info_t cpu_info[8];
uint8_t cpu_info_index = 0;

global_uint64_t opcode;
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

global_uint64_t cpu_unsigned2signed(global_uint64_t value, uint8_t size)
{
    int8_t signed8;
    int16_t signed16;
    int32_t signed32;
    int32_t signed64;
    switch (size)
    {
    case 1:
        signed8 = (int8_t)value;
        return (global_int64_t)signed8;
    case 2:
        signed16 = (int16_t)value;
        return (global_int64_t)signed16;
    case 4:
        signed32 = (int32_t)value;
        return (global_int64_t)signed32;
    case 8:
        signed64 = (global_int64_t)value;
        return (global_int64_t)signed32;
    }
    return value;
}

global_uint64_t cpu_read_reg(uint8_t reg)
{
    if (reg == cpu_reg_ip)
        reg = cpu_reg_eip;
    if (reg >= cpu_reg_gdtr && reg < cpu_reg_end && ((reg - cpu_reg_gdtr) & 8) == 0)
    {
        return *(global_uint64_t *)&cpu_state[reg];
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
    if (reg >= cpu_reg_al && reg <= cpu_reg_r15b)
    {
        return *(uint8_t *)&cpu_state[reg];
    }
    if (reg >= cpu_reg_gs && reg <= cpu_reg_ss)
    {
        return *(uint16_t *)&cpu_state[reg];
    }
    return 0;
}

void cpu_write_reg(uint8_t reg, global_uint64_t value)
{
    if (reg == cpu_reg_ip)
        reg = cpu_reg_eip;
    if (reg >= cpu_reg_gdtr && reg < cpu_reg_end && ((reg - cpu_reg_gdtr) & 8) == 0)
    {
        *(global_uint64_t *)&cpu_state[reg] = value;
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
    if (reg >= cpu_reg_al && reg <= cpu_reg_r15b)
    {
        *(uint8_t *)&cpu_state[reg] = value & 0xff;
        return;
    }
    if (reg >= cpu_reg_gs && reg <= cpu_reg_ss)
    {
        *(uint16_t *)&cpu_state[reg] = value & 0xffff;
        return;
    }
}

static inline void cpu_add_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) + value);
}

static inline void cpu_sub_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) - value);
}

static inline void cpu_mul_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) * value);
}

static inline void cpu_div_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) / value);
}

static inline void cpu_and_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) & value);
}

static inline void cpu_or_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) | value);
}

static inline void cpu_xor_reg(uint8_t reg, global_uint64_t value)
{
    cpu_write_reg(reg, cpu_read_reg(reg) ^ value);
}

static inline global_uint64_t cpu_resolve_value(global_uint64_t value, uint8_t index, uint8_t size)
{
    gdt_entry_t *entry = (gdt_entry_t *)(&vm_memory[((gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)])->base] +
                                         cpu_read_reg(cpu_reg_ds));
    global_uint64_t base = (entry->base_high << 24) |
                           (entry->base_middle << 16) |
                           entry->base_low;
    global_uint64_t buffer_address = 0;
    switch (cpu_info[index].reg_type)
    {
    case cpu_type_memory:
        return memory_read(base + value, size, 0);
    case cpu_type_memory_reg:
        for (uint8_t i = 0; i < cpu_info[index].reg_type_buffer[0]; i++)
        {
            switch (cpu_info[index].reg_type_buffer[i + 1])
            {
            case cpu_reg_esp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_sp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_ebp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_bp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            }
            buffer_address += cpu_read_reg(cpu_info[index].reg_type_buffer[i + 1]);
        }
        if (cpu_info[index].reg_type_buffer[0])
            return memory_read(base + buffer_address +
                                   (cpu_info[index].sign ? cpu_unsigned2signed(value, cpu_info[index].size) : value),
                               size, 0);
        else
            return memory_read(base + cpu_read_reg(value), size, 0);
    case cpu_type_reg:
        return cpu_read_reg(value);
    };
    return value;
}

static inline global_uint64_t cpu_resolve_address(global_uint64_t value, uint8_t index, uint8_t size)
{
    global_uint64_t buffer_address = 0;
    switch (cpu_info[index].reg_type)
    {
    case cpu_type_memory_reg:
        for (uint8_t i = 0; i < cpu_info[index].reg_type_buffer[0]; i++)
        {
            switch (cpu_info[index].reg_type_buffer[i + 1])
            {
            case cpu_reg_esp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_sp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_ebp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_bp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            }
            buffer_address += cpu_read_reg(cpu_info[index].reg_type_buffer[i + 1]);
        }
        if (cpu_info[index].reg_type_buffer[0])
            return buffer_address + (cpu_info[index].sign ? cpu_unsigned2signed(value, cpu_info[index].size) : value);
        else
            return cpu_read_reg(value);
    };
    return value;
}

static inline void cpu_exec_mov(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    value2 = cpu_resolve_value(value2, 1, size);
    global_uint64_t base = gdt_read_seg_offset(cpu_reg_ds);
    global_uint64_t buffer_address = 0;
    switch (cpu_info[0].reg_type)
    {
    case cpu_type_memory:
        memory_write(base + value1, value2, size, 0);
        break;
    case cpu_type_memory_reg:
        for (uint8_t i = 0; i < cpu_info[0].reg_type_buffer[0]; i++)
        {
            switch (cpu_info[0].reg_type_buffer[i + 1])
            {
            case cpu_reg_esp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_sp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_ebp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_bp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            }
            buffer_address += cpu_read_reg(cpu_info[0].reg_type_buffer[i + 1]);
        }
        if (cpu_info[0].reg_type_buffer[0])
            memory_write(base + buffer_address +
                             (cpu_info[0].sign ? cpu_unsigned2signed(value1, cpu_info[0].size) : value1),
                         value2, size, 0);
        else
            memory_write(base + cpu_read_reg(value1), value2, size, 0);
        break;
    default:
        cpu_write_reg(value1, value2);
        break;
    };
}

static inline void cpu_exec_lea(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    value2 = cpu_resolve_address(value2, 1, size);
    global_uint64_t base = gdt_read_seg_offset(cpu_reg_ds);
    global_uint64_t buffer_address = 0;
    switch (cpu_info[0].reg_type)
    {
    case cpu_type_memory:
        memory_write(base + value1, value2, size, 0);
        break;
    case cpu_type_memory_reg:
        for (uint8_t i = 0; i < cpu_info[0].reg_type_buffer[0]; i++)
        {
            switch (cpu_info[0].reg_type_buffer[i + 1])
            {
            case cpu_reg_esp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_sp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_ebp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            case cpu_reg_bp:
                buffer_address += gdt_read_seg_offset(cpu_reg_ss);
                break;
            }
            buffer_address += cpu_read_reg(cpu_info[0].reg_type_buffer[i + 1]);
        }
        if (cpu_info[0].reg_type_buffer[0])
            memory_write(base + buffer_address +
                             (cpu_info[0].sign ? cpu_unsigned2signed(value1, cpu_info[0].size) : value1),
                         value2, size, 0);
        else
            memory_write(base + cpu_read_reg(value1), value2, size, 0);
        break;
    default:
        cpu_write_reg(value1, value2);
        break;
    };
}

static inline void cpu_exec_add(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1,
                 resolved_value1 + (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2),
                 size);
}

static inline void cpu_exec_sub(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 - (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
}

static inline void cpu_exec_mul(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 * (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
}

static inline void cpu_exec_div(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    if (value2 != 0)
        cpu_exec_mov(value1, resolved_value1 / (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
    else
        cpu_exec_mov(value1, 0, size);
}

static inline void cpu_exec_and(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 & (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
}

static inline void cpu_exec_or(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 | (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
}

static inline void cpu_exec_xor(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 ^ (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
}

static inline void cpu_exec_shl(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 << (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
}

static inline void cpu_exec_shr(global_uint64_t value1, global_uint64_t value2, uint8_t size)
{
    global_uint64_t resolved_value1 = cpu_resolve_value(value1, 0, size);
    value2 = cpu_resolve_value(value2, 1, size);
    cpu_info[1].reg_type = cpu_type_int;
    cpu_exec_mov(value1, resolved_value1 >> (cpu_info[1].sign ? cpu_unsigned2signed(value2, cpu_info[1].size) : value2), size);
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
    return cpu_reg_eflags;
}

static inline void cpu_exec_cmp(global_uint64_t value1, global_uint64_t value2, uint8_t size)
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

static inline void cpu_exec_jmp_near(uint8_t reg, global_uint64_t value1, uint8_t size)
{
    value1 = cpu_resolve_value(value1, 0, size);
    cpu_add_reg(reg, value1 - cpu_read_reg(reg));
}

static inline void cpu_exec_jif_near(uint8_t reg, uint8_t condition, global_uint64_t value1, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    if (cpu_read_reg(flags) & condition)
        cpu_exec_jmp_near(reg, value1, size);
    else
        cpu_add_reg(reg, 1);
}

static inline void cpu_exec_jnif_near(uint8_t reg, uint8_t condition, global_uint64_t value1, uint8_t size)
{
    uint8_t flags = cpu_resolve_flags(size);
    if (~cpu_read_reg(flags) & condition)
        cpu_exec_jmp_near(reg, value1, size);
    else
        cpu_add_reg(reg, 1);
}

static inline void cpu_push_reg(uint8_t stack, uint8_t reg, uint8_t size)
{
    global_uint64_t base = gdt_read_seg_offset(cpu_reg_ss);
    cpu_write_reg(stack, cpu_read_reg(stack) - size);
    switch (size)
    {
    case 1:
        *((uint8_t *)&vm_memory[base + cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    case 2:
        *((uint16_t *)&vm_memory[base + cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    case 4:
        *((uint32_t *)&vm_memory[base + cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    default:
        *((global_uint64_t *)&vm_memory[base + cpu_read_reg(stack)]) = cpu_read_reg(reg);
        break;
    }
}

static inline void cpu_push_int(uint8_t stack, global_uint64_t value, uint8_t size)
{
    global_uint64_t base = gdt_read_seg_offset(cpu_reg_ss);
    cpu_write_reg(stack, cpu_read_reg(stack) - size);
    switch (size)
    {
    case 1:
        *((uint8_t *)&vm_memory[base + cpu_read_reg(stack)]) = value;
        break;
    case 2:
        *((uint16_t *)&vm_memory[base + cpu_read_reg(stack)]) = value;
        break;
    case 4:
        *((uint32_t *)&vm_memory[base + cpu_read_reg(stack)]) = value;
        break;
    default:
        *((global_uint64_t *)&vm_memory[base + cpu_read_reg(stack)]) = value;
        break;
    }
}

static inline void cpu_pop_reg(uint8_t stack, uint8_t reg, uint8_t size)
{
    global_uint64_t base = gdt_read_seg_offset(cpu_reg_ss);
    switch (size)
    {
    case 1:
        cpu_write_reg(reg, *((uint8_t *)&vm_memory[base + cpu_read_reg(stack)]));
        break;
    case 2:
        cpu_write_reg(reg, *((uint16_t *)&vm_memory[base + cpu_read_reg(stack)]));
        break;
    case 4:
        cpu_write_reg(reg, *((uint32_t *)&vm_memory[base + cpu_read_reg(stack)]));
        break;
    default:
        cpu_write_reg(reg, *((global_uint64_t *)&vm_memory[base + cpu_read_reg(stack)]));
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
            printf("%s: 0x%llx,", cpu_regs_string[i], cpu_read_reg(i));
    printf("\n};\n");
}

static inline void cpu_rm_resolve(uint8_t reg, uint8_t rm8, uint8_t size)
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
    case 4:
        switch (rm8)
        {
        case 0x00:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebx;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_esi;
            break;
        case 0x01:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebx;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_edi;
            break;
        case 0x02:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebp;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_esi;
            break;
        case 0x03:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebp;
            cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_edi;
            break;
        case 0x04:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            switch (memory_read(opcode + 2, 1, 0))
            {
            case 0x08:
                cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
                cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_eax;
                cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_ecx;
                opcode++, cpu_add_reg(reg, 1);
                break;
            case 0x24:
                cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
                cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_esp;
                opcode++, cpu_add_reg(reg, 1);
                break;
            }
            break;
        case 0x05:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebp;
            break;
        case 0x06:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebp;
            break;
        case 0x07:
            cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
            cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
            cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_ebx;
            break;
        }
        break;
    default:
        break;
    }
}

static inline global_uint64_t cpu_rm(uint8_t reg, uint8_t size, uint8_t override_size)
{
    uint8_t mod = (memory_read(opcode + 1, 1, 0) & 0xc0) >> 0x06;
    uint8_t rm8 = memory_read(opcode + 1, 1, 0) & 0x07;
    global_int64_t value = 0;
    switch (mod)
    {
    case 0x00:
        switch (rm8)
        {
        case 0x00:
            switch (size)
            {
            case 1:
                value = regs8[rm8];
                cpu_info[cpu_info_index].reg_type_buffer[0] = 0;
                cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
                break;
            case 2:
                value = regs16[rm8];
                cpu_info[cpu_info_index].reg_type_buffer[0] = 0;
                cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
                break;
            case 4:
                value = regs32[rm8];
                cpu_info[cpu_info_index].reg_type_buffer[0] = 0;
                cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
                break;
            }
            goto ret;
        case 0x05 ... 0x06:
            switch (override_size)
            {
            case 1 ... 2:
                value = memory_read(opcode + 2, 2, 0);
                override_size = 2;
                break;
            case 4:
                value = memory_read(opcode + 2, 4, 0);
                break;
            default:
                break;
            }
            opcode += override_size, cpu_add_reg(reg, override_size);
            cpu_info[cpu_info_index].reg_type = cpu_type_memory;
            cpu_info[cpu_info_index].size = override_size;
            cpu_info[cpu_info_index].sign = 1;
            goto ret;
        }
        cpu_rm_resolve(reg, rm8, size);
        break;
    case 0x01:
        cpu_rm_resolve(reg, rm8, size);
        value = cpu_unsigned2signed(memory_read(opcode + 2, 1, 0), 1);
        cpu_info[cpu_info_index].size = 1;
        cpu_info[cpu_info_index].sign = 1;
        opcode++, cpu_add_reg(reg, 1);
        break;
    case 0x02:
        cpu_rm_resolve(reg, rm8, size);
        switch (override_size)
        {
        case 1 ... 2:
            value = memory_read(opcode + 2, 2, 0);
            override_size = 2;
            break;
        case 4:
            value = memory_read(opcode + 2, 4, 0);
            break;
        default:
            break;
        }
        opcode += override_size, cpu_add_reg(reg, override_size);
        cpu_info[cpu_info_index].size = override_size;
        cpu_info[cpu_info_index].sign = 1;
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
ret:
    opcode++, cpu_add_reg(reg, 1), cpu_info_index++;
    return value;
}

static inline global_uint64_t cpu_r_rm(uint8_t reg, void *r, uint8_t r_size, uint8_t size, uint8_t override_size)
{
    global_uint64_t opcode_backup = opcode;
    switch (r_size)
    {
    case 1:
        *(global_uint64_t *)r = regs8[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    case 2:
        *(global_uint64_t *)r = regs16[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    case 4:
        *(global_uint64_t *)r = regs32[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    case 255:
        *(global_uint64_t *)r = segregs[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    default:
        break;
    }
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    global_uint64_t rm = cpu_rm(reg, size, override_size);
    return rm;
}

static inline global_uint64_t cpu_rm_r(uint8_t reg, void *r, uint8_t r_size, uint8_t size, uint8_t override_size)
{
    global_uint64_t opcode_backup = opcode;
    switch (r_size)
    {
    case 1:
        *(global_uint64_t *)r = regs8[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    case 2:
        *(global_uint64_t *)r = regs16[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    case 4:
        *(global_uint64_t *)r = regs32[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    case 255:
        *(global_uint64_t *)r = segregs[(memory_read(opcode_backup + 1, 1, 0) & 0x38) >> 3];
        break;
    default:
        break;
    }
    global_uint64_t rm = cpu_rm(reg, size, override_size);
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
    return cpu_override_ds;
}

static inline global_uint64_t cpu_m(uint8_t reg, uint16_t override, uint8_t size)
{
    global_uint64_t value = 0;
    uint8_t sreg = cpu_get_segment_override(override);
    cpu_info[cpu_info_index].segmentation = sreg;
    cpu_info[cpu_info_index++].reg_type = cpu_type_memory;
    value = memory_read(opcode + 1, size, 0);
    opcode += size, cpu_add_reg(reg, size);
    return value;
}

static inline uint16_t cpu_m16(uint8_t reg, uint16_t override)
{
    return (uint16_t)cpu_m(reg, override, 2);
}

static inline uint32_t cpu_m32(uint8_t reg, uint16_t override)
{
    return (uint32_t)cpu_m(reg, override, 4);
}

static inline uint8_t cpu_r32(uint8_t reg)
{
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return regs32[(memory_read(opcode, 4, 0) & 0x38) & 7];
}

static inline uint8_t cpu_r16(uint8_t reg)
{
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return regs16[(memory_read(opcode, 2, 0) & 0x38) & 7];
}

static inline uint8_t cpu_r8(uint8_t reg)
{
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    return regs8[(memory_read(opcode, 1, 0) & 0x38) & 7];
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

static inline uint32_t cpu_r32_rm16(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_r_rm(reg, r, 4, 2, override_size);
}

static inline uint32_t cpu_r32_rm8(uint8_t reg, void *r, uint8_t override_size)
{
    return (uint32_t)cpu_r_rm(reg, r, 4, 1, override_size);
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

static inline uint32_t cpu_rm32(uint8_t reg, uint8_t override_size)
{
    return (uint32_t)cpu_rm(reg, 4, override_size);
}

static inline uint16_t cpu_rm16(uint8_t reg, uint8_t override_size)
{
    return (uint16_t)cpu_rm(reg, 2, override_size);
}

static inline uint8_t cpu_rm8(uint8_t reg, uint8_t override_size)
{
    return (uint8_t)cpu_rm(reg, 1, override_size);
}

static inline uint32_t cpu_imm32(uint8_t reg, uint8_t sign)
{
    uint32_t imm = memory_read(opcode + 1, 4, 0);
    cpu_info[cpu_info_index].size = 4;
    cpu_info[cpu_info_index].sign = sign;
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 4);
    return (uint32_t)imm;
}

static inline uint16_t cpu_imm16(uint8_t reg, uint8_t sign)
{
    uint16_t imm = memory_read(opcode + 1, 2, 0);
    cpu_info[cpu_info_index].size = 2;
    cpu_info[cpu_info_index].sign = sign;
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 2);
    return (uint16_t)imm;
}

static inline uint8_t cpu_imm8(uint8_t reg, uint8_t sign)
{
    uint8_t imm = memory_read(opcode + 1, 1, 0);
    cpu_info[cpu_info_index].size = 1;
    cpu_info[cpu_info_index].sign = sign;
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    return (uint8_t)imm;
}

static inline uint32_t cpu_rel32(uint8_t reg)
{
    uint32_t rel = memory_read(opcode + 1, 4, 0);
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 4);
    return (uint32_t)(cpu_read_reg(reg) + rel + 1);
}

static inline uint16_t cpu_rel16(uint8_t reg)
{
    uint16_t rel = memory_read(opcode + 1, 2, 0);
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode += 2, cpu_write_reg(reg, cpu_read_reg(reg) + 2);
    return (uint16_t)(cpu_read_reg(reg) + rel + 1);
}

static inline uint8_t cpu_rel8(uint8_t reg)
{
    uint8_t rel = memory_read(opcode + 1, 1, 0);
    cpu_info[cpu_info_index++].reg_type = cpu_type_int;
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    return (uint8_t)(cpu_read_reg(reg) + rel + 1);
}

static inline uint8_t cpu_sr(uint8_t reg)
{
    cpu_info[cpu_info_index++].reg_type = cpu_type_reg;
    opcode++, cpu_write_reg(reg, cpu_read_reg(reg) + 1);
    return segregs[(memory_read(opcode + 1, 1, 0) & 0x38) >> 3];
}

static inline void cpu_print_instruction(char *instruction, char *type,
                                         uint8_t regs, ...)
{
    global_int64_t cache;
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
        global_uint64_t value = va_arg(args, global_uint64_t);
        if (index == print_pos)
            printf("%s ", type);
        switch (cpu_info[index].reg_type)
        {
        case cpu_type_reg:
            printf("%s", cpu_regs_string[value]);
            break;
        case cpu_type_int:
            cache = cpu_unsigned2signed(value, cpu_info[index].size);
            if (cache > -1)
                printf("%s 0x%llx", cpu_sizes_string[cpu_info[index].size], cache);
            else if (cpu_info[index].sign)
                printf("%s -0x%llx", cpu_sizes_string[cpu_info[index].size], -cache);
            else
                printf("%s 0x%llx", cpu_sizes_string[cpu_info[index].size], value);
            break;
        case cpu_type_memory:
            printf("[%s 0x%llx]", cpu_sizes_string[cpu_info[index].size], value);
            break;
        case cpu_type_memory_reg:
            cache = cpu_unsigned2signed(value, cpu_info[index].size);
            if (cpu_info[index].reg_type_buffer[0])
            {
                printf("[");
                for (uint8_t i = 0; i < cpu_info[index].reg_type_buffer[0]; i++)
                {
                    printf("%s", cpu_regs_string[cpu_info[index].reg_type_buffer[i + 1]]);
                    if (cpu_info[index].reg_type_buffer[0] - i > 1)
                        printf(" + ");
                    else
                        break;
                }
                if (cache > -1)
                    printf(" + 0x%llx]", cache);
                else if (cpu_info[index].sign)
                    printf(" - 0x%llx]", -cache);
                else
                    printf(" + 0x%llx]", value);
            }
            else
            {
                printf("[%s]", cpu_regs_string[value]);
            }
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
    global_uint64_t base = gdt_read_seg_offset(cpu_reg_cs);
    opcode = base + cpu_read_reg(cpu_reg_ip);
    global_uint64_t value1;
    global_uint64_t value2;
    uint8_t size;
    uint8_t opcode_byte = memory_read(opcode, 1, 0);
    char *operation;
    switch (opcode_byte)
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
            printf("OPCODE: 0x%x, ESP: 0x%llx; ", opcode_byte, cpu_read_reg(cpu_reg_esp));
        break;
    }
    switch (opcode_byte)
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
    case 0x08:
        value1 = cpu_rm8_r8(cpu_reg_ip, &value2, 1);
        cpu_exec_or(value1, value2, 1);
        if (debug)
            cpu_print_instruction("or", "byte", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x09:
        if (override & cpu_override_dword_operand)
        {
            value1 = cpu_rm32_r32(cpu_reg_ip, &value2, 2);
            cpu_exec_or(value1, value2, 1);
            if (debug)
                cpu_print_instruction("or", "dword", 2, value1, value2);
        }
        else
        {
            if (override & cpu_override_dword_address)
            {
                value1 = cpu_rm32_r16(cpu_reg_ip, &value2, 4);
                cpu_exec_or(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("or", "word", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm16_r16(cpu_reg_ip, &value2, 2);
                cpu_exec_or(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("or", "word", 2, value1, value2);
            }
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x0a:
        value2 = cpu_r8_rm8(cpu_reg_ip, &value1, 1);
        cpu_exec_or(value1, value2, 1);
        if (debug)
            cpu_print_instruction("or", "byte", 2, value1, value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x0b:
        if (override & cpu_override_dword_operand)
        {
            value2 = cpu_r32_rm32(cpu_reg_ip, &value1, 2);
            cpu_exec_or(value1, value2, 1);
            if (debug)
                cpu_print_instruction("or", "dword", 2, value1, value2);
        }
        else
        {
            if (override & cpu_override_dword_address)
            {
                value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
                cpu_exec_or(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("or", "dword", 2, value1, value2);
            }
            else
            {
                value2 = cpu_r16_rm16(cpu_reg_ip, &value1, 2);
                cpu_exec_or(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("or", "word", 2, value1, value2);
            }
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x0c:
        value1 = cpu_imm8(cpu_reg_ip, 0);
        cpu_or_reg(cpu_reg_al, value1);
        if (debug)
            cpu_print_instruction("or", "byte al,", 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x0d:
        if (override & cpu_override_dword_operand)
        {
            value1 = cpu_imm32(cpu_reg_ip, 0);
            cpu_or_reg(cpu_reg_eax, value1);
            if (debug)
                cpu_print_instruction("or", "dword eax,", 1, value1);
        }
        else
        {
            value1 = cpu_imm16(cpu_reg_ip, 0);
            cpu_or_reg(cpu_reg_ax, value1);
            if (debug)
                cpu_print_instruction("or", "word ax,", 1, value1);
        }
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
        switch ((memory_read(opcode, 1, 0) & 0x38) >> 3)
        {
        case 0x00:
            switch (memory_read(opcode, 1, 0) & 0x07)
            {
            case 0x01:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jnif_near(cpu_reg_ip, cpu_flags_CF, value1, 2);
                if (debug)
                    printf("jnc 0x%llx\n", value1);
                break;
            case 0x02:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jif_near(cpu_reg_ip, cpu_flags_CF, value1, 2);
                if (debug)
                    printf("jc 0x%llx\n", value1);
                break;
            case 0x03:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jnif_near(cpu_reg_ip, cpu_flags_ZF, value1, 2);
                if (debug)
                    printf("jnz 0x%llx\n", value1);
                break;
            case 0x04:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jif_near(cpu_reg_ip, cpu_flags_ZF, value1, 2);
                if (debug)
                    printf("jz 0x%llx\n", value1);
                break;
            case 0x05:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jnif_near(cpu_reg_ip, cpu_flags_ZF, value1, 2);
                if (debug)
                    printf("jne 0x%llx\n", value1);
                break;
            case 0x06:
                value1 = cpu_rel16(cpu_reg_ip);
                cpu_exec_jif_near(cpu_reg_ip, cpu_flags_ZF, value1, 2);
                if (debug)
                    printf("je 0x%llx\n", value1);
                break;
            default:
                goto is_pop_cs;
            }
            break;
        case 0x01:
            value1 = cpu_rel16(cpu_reg_ip);
            cpu_exec_jnif_near(cpu_reg_ip, cpu_flags_CF, value1, 2);
            if (debug)
                printf("jnl 0x%llx\n", value1);
            break;
        case 0x05:
            if (override & cpu_override_dword_operand)
                value1 = cpu_rm32_r32(cpu_reg_ip, &value2, 4), size = 4;
            else
                value1 = cpu_rm16_r16(cpu_reg_ip, &value2, 2), size = 2;
            cpu_exec_mul(value1, value2, 1);
            if (debug)
                cpu_print_instruction("imul", cpu_sizes_string[size], 2, value1, value2);
            cpu_add_reg(cpu_reg_ip, 1);
            break;
        case 0x06:
            if (override & cpu_override_dword_operand)
            {
                if (override & cpu_override_dword_address)
                    value2 = cpu_r32_rm32(cpu_reg_ip, &value1, 4);
                else
                    value2 = cpu_r32_rm8(cpu_reg_ip, &value1, 2);
            }
            else
            {
                if (override & cpu_override_dword_address)
                    value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
                else
                    value2 = cpu_r16_rm8(cpu_reg_ip, &value1, 2);
            }
            cpu_exec_mov(value1, value2, 1);
            if (debug)
                cpu_print_instruction("movzx", "1byte", 2, value1, value2);
            cpu_add_reg(cpu_reg_ip, 1);
            break;
        case 0x07:
            if (override & cpu_override_dword_operand)
            {
                if (override & cpu_override_dword_address)
                    value2 = cpu_r32_rm32(cpu_reg_ip, &value1, 4);
                else
                    value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
            }
            else
            {
                if (override & cpu_override_dword_address)
                    value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
                else
                    value2 = cpu_r16_rm16(cpu_reg_ip, &value1, 2);
            }
            cpu_exec_mov(value1, value2, 2);
            if (debug)
                cpu_print_instruction("movzx", "1word", 2, value1, value2);
            cpu_add_reg(cpu_reg_ip, 1);
            break;
        is_pop_cs:
        default:
            cpu_pop_reg(cpu_reg_sp, cpu_reg_cs, 2);
            if (debug)
                printf("pop word cs\n");
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
    case 0x50 ... 0x57:
        if (override & cpu_override_dword_operand)
        {
            value1 = regs32[opcode_byte & 0x07];
            cpu_push_reg(cpu_reg_sp, value1, 4);
            if (debug)
                printf("push dword %s\n", cpu_regs_string[value1]);
        }
        else
        {
            value1 = regs16[opcode_byte & 0x07];
            cpu_push_reg(cpu_reg_sp, value1, 2);
            if (debug)
                printf("push word %s\n", cpu_regs_string[value1]);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x58 ... 0x5f:
        if (override & cpu_override_dword_operand)
        {
            value1 = regs32[opcode_byte & 0x07];
            cpu_pop_reg(cpu_reg_sp, value1, 4);
            if (debug)
                printf("pop dword %s\n", cpu_regs_string[value1]);
        }
        else
        {
            value1 = regs16[opcode_byte & 0x07];
            cpu_pop_reg(cpu_reg_sp, value1, 2);
            if (debug)
                printf("pop word %s\n", cpu_regs_string[value1]);
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
        value2 = cpu_imm8(cpu_reg_ip, 0);
        cpu_push_int(cpu_reg_sp, value2, 2);
        if (debug)
            printf("push byte 0x%llx\n", value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x68:
        value2 = cpu_imm16(cpu_reg_ip, 0);
        cpu_push_int(cpu_reg_sp, value2, 2);
        if (debug)
            printf("push word 0x%llx\n", value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x74:
        value1 = cpu_rel8(cpu_reg_ip);
        cpu_exec_jif_near(cpu_reg_ip, cpu_flags_ZF, value1, 2);
        if (debug)
            printf("je 0x%llx\n", value1);
        break;
    case 0x80:
        opcode_byte = memory_read(opcode + 1, 1, 0);
        if (override & cpu_override_dword_address)
            value1 = cpu_rm32(cpu_reg_ip, 4);
        else
            value1 = cpu_rm8(cpu_reg_ip, 1);
        value2 = cpu_imm8(cpu_reg_ip, 1);
        switch ((opcode_byte & 0x38) >> 3)
        {
        case 0x00:
            cpu_exec_add(value1, value2, 1);
            if (debug)
                cpu_print_instruction("add", "byte", 2, value1, value2);
            break;
        case 0x01:
            cpu_exec_or(value1, value2, 1);
            if (debug)
                cpu_print_instruction("or", "byte", 2, value1, value2);
            break;
        case 0x04:
            cpu_exec_and(value1, value2, 1);
            if (debug)
                cpu_print_instruction("and", "byte", 2, value1, value2);
            break;
        case 0x05:
            cpu_exec_sub(value1, value2, 1);
            if (debug)
                cpu_print_instruction("sub", "byte", 2, value1, value2);
            break;
        case 0x06:
            cpu_exec_xor(value1, value2, 1);
            if (debug)
                cpu_print_instruction("xor", "byte", 2, value1, value2);
            break;
        case 0x07:
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
        opcode_byte = memory_read(opcode + 1, 1, 0);
        if (override & cpu_override_dword_address)
            value1 = cpu_rm32(cpu_reg_ip, 4), size = 4;
        else
            value1 = cpu_rm16(cpu_reg_ip, 2), size = 4;
        if (override & cpu_override_dword_operand)
            value2 = cpu_imm32(cpu_reg_ip, 1);
        else
            value2 = cpu_imm16(cpu_reg_ip, 1);
        switch ((opcode_byte & 0x38) >> 3)
        {
        case 0x00:
            cpu_exec_add(value1, value2, size);
            if (debug)
                cpu_print_instruction("add", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x01:
            cpu_exec_or(value1, value2, size);
            if (debug)
                cpu_print_instruction("or", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x04:
            cpu_exec_and(value1, value2, size);
            if (debug)
                cpu_print_instruction("and", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x05:
            cpu_exec_sub(value1, value2, size);
            if (debug)
                cpu_print_instruction("sub", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x06:
            cpu_exec_xor(value1, value2, size);
            if (debug)
                cpu_print_instruction("xor", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x07:
            cpu_exec_cmp(value1, value2, size);
            if (debug)
                cpu_print_instruction("cmp", cpu_sizes_string[size], 2, value1, value2);
            break;
        default:
            goto unknown;
            break;
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x83:
        opcode_byte = memory_read(opcode + 1, 1, 0);
        if (override & cpu_override_dword_operand)
            value1 = cpu_rm32(cpu_reg_ip, 4), size = 4;
        else
            value1 = cpu_rm16(cpu_reg_ip, 2), size = 2;
        value2 = cpu_imm8(cpu_reg_ip, 1);
        switch ((opcode_byte & 0x38) >> 3)
        {
        case 0x00:
            cpu_exec_add(value1, value2, 1);
            if (debug)
                cpu_print_instruction("add", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x01:
            cpu_exec_or(value1, value2, 4);
            if (debug)
                cpu_print_instruction("or", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x04:
            cpu_exec_and(value1, value2, 4);
            if (debug)
                cpu_print_instruction("and", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x05:
            cpu_exec_sub(value1, value2, 4);
            if (debug)
                cpu_print_instruction("sub", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x06:
            cpu_exec_xor(value1, value2, 4);
            if (debug)
                cpu_print_instruction("xor", cpu_sizes_string[size], 2, value1, value2);
            break;
        case 0x07:
            cpu_exec_cmp(value1, value2, 4);
            if (debug)
                cpu_print_instruction("cmp", cpu_sizes_string[size], 2, value1, value2);
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
            if (override & cpu_override_dword_address)
                value1 = cpu_rm32_r16(cpu_reg_ip, &value2, 4);
            else
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
    case 0x8d:
        if (override & cpu_override_dword_operand)
        {
            if (override & cpu_override_dword_address)
                value2 = cpu_r32_rm32(cpu_reg_ip, &value1, 4);
            else
                value2 = cpu_r32_rm16(cpu_reg_ip, &value1, 2);
            cpu_exec_lea(value1, value2, 4);
            if (debug)
                cpu_print_instruction("lea", "dword", 2, value1, value2);
        }
        else
        {
            if (override & cpu_override_dword_address)
                value2 = cpu_r16_rm32(cpu_reg_ip, &value1, 4);
            else
                value2 = cpu_r16_rm16(cpu_reg_ip, &value1, 2);
            cpu_exec_lea(value1, value2, 2);
            if (debug)
                cpu_print_instruction("lea", "word", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0x90:
        if (debug)
            printf("nop\n");
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xa0:
        if (override & cpu_override_dword_address)
            value1 = cpu_m32(cpu_reg_ip, override);
        else
            value1 = cpu_m16(cpu_reg_ip, override);
        value2 = cpu_resolve_value(value1, 0, 1);
        cpu_write_reg(cpu_reg_al, value2);
        if (debug)
            printf("mov byte al, [%s 0x%llx]\n",
                   override & cpu_override_dword_address ? "dword" : "word",
                   value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xa1:
        if (override & cpu_override_dword_address)
            value1 = cpu_m32(cpu_reg_ip, override);
        else
            value1 = cpu_m16(cpu_reg_ip, override);
        if (override & cpu_override_dword_operand)
        {
            value2 = cpu_resolve_value(value1, 0, 4);
            cpu_write_reg(cpu_reg_eax, value2);
            if (debug)
                printf("mov dword eax, [%s 0x%llx]\n",
                       override & cpu_override_dword_address ? "dword" : "word",
                       value1);
        }
        else
        {
            value2 = cpu_resolve_value(value1, 0, 2);
            cpu_write_reg(cpu_reg_ax, value2);
            if (debug)
                printf("mov word ax, [%s 0x%llx]\n",
                       override & cpu_override_dword_address ? "dword" : "word",
                       value1);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb0 ... 0xb7:
        value1 = regs8[opcode_byte & 0x07];
        cpu_write_reg(value1, cpu_imm8(cpu_reg_ip, 0));
        if (debug)
            printf("mov byte %s, 0x%llx\n", cpu_regs_string[value1], cpu_read_reg(value1));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xb8 ... 0xbf:
        if (override & cpu_override_dword_operand)
        {
            value1 = regs32[opcode_byte & 0x07];
            cpu_write_reg(value1, cpu_imm32(cpu_reg_ip, 0));
            if (debug)
                printf("mov dword %s, 0x%llx\n", cpu_regs_string[value1], cpu_read_reg(value1));
        }
        else
        {
            value1 = regs16[opcode_byte & 0x07];
            cpu_write_reg(value1, cpu_imm16(cpu_reg_ip, 0));
            if (debug)
                printf("mov word %s, 0x%llx\n", cpu_regs_string[value1], cpu_read_reg(value1));
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xc1:
        if (override & cpu_override_dword_operand)
            value1 = cpu_rm32(cpu_reg_ip, 4), size = 4;
        else
            value1 = cpu_rm16(cpu_reg_ip, 2), size = 2;
        value2 = cpu_imm8(cpu_reg_ip, 0);
        cpu_exec_shl(value1, value2, 1);
        if (debug)
            cpu_print_instruction("shl", cpu_sizes_string[size], 2, value1, value2);
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
            value1 = cpu_rm32(cpu_reg_ip, 4);
            value2 = cpu_imm8(cpu_reg_ip, 0);
            cpu_exec_mov(value1, value2, 1);
            if (debug)
                cpu_print_instruction("mov", "byte", 2, value1, value2);
        }
        else
        {
            value1 = cpu_rm16(cpu_reg_ip, 2);
            value2 = cpu_imm8(cpu_reg_ip, 0);
            cpu_exec_mov(value1, value2, 1);
            if (debug)
                cpu_print_instruction("mov", "byte", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xc7:
        if (override & cpu_override_dword_address)
        {
            if (override & cpu_override_dword_operand)
            {
                value1 = cpu_rm32(cpu_reg_ip, 4);
                value2 = cpu_imm32(cpu_reg_ip, 0);
                cpu_exec_mov(value1, value2, 4);
                if (debug)
                    cpu_print_instruction("mov", "dword", 2, value1, value2);
            }
            else
            {
                value1 = cpu_rm32(cpu_reg_ip, 2);
                value2 = cpu_imm16(cpu_reg_ip, 0);
                cpu_exec_mov(value1, value2, 2);
                if (debug)
                    cpu_print_instruction("mov", "word", 2, value1, value2);
            }
        }
        else
        {
            value1 = cpu_rm16(cpu_reg_ip, 2);
            value2 = cpu_imm16(cpu_reg_ip, 0);
            cpu_exec_mov(value1, value2, 2);
            if (debug)
                cpu_print_instruction("mov", "word", 2, value1, value2);
        }
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xcd:
        value2 = cpu_imm8(cpu_reg_ip, 0);
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
            memory_write(0xfff0, window_framebuffer[1], 2, 0);
            memory_write(0xfff2, window_framebuffer[2], 2, 0);
            memory_write(0xfff4, window_framebuffer[3], 2, 0);
            break;
        default:
            break;
        }
        if (debug)
            printf("int byte 0x%llx\n", (unsigned long long)value2);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe4:
        value1 = cpu_imm8(cpu_reg_ip, 0);
        cpu_write_reg(cpu_reg_al, io_read(value1, 1));
        if (debug)
            cpu_print_instruction("in al,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe5:
        value1 = cpu_imm8(cpu_reg_ip, 0);
        cpu_write_reg(cpu_reg_ax, io_read(value1, 2));
        if (debug)
            cpu_print_instruction("in ax,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe6:
        value1 = cpu_imm8(cpu_reg_ip, 0);
        io_write(cpu_read_reg(cpu_reg_al), value1, 1);
        if (debug)
            cpu_print_instruction("out al,", (void *)NULL, 1, value1);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xe7:
        value1 = cpu_imm8(cpu_reg_ip, 0);
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
                printf("call dword 0x%llx\n", value1);
        }
        else
        {
            value1 = cpu_rel16(cpu_reg_ip);
            cpu_add_reg(cpu_reg_ip, 1);
            cpu_push_reg(cpu_reg_sp, cpu_reg_ip, 2);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("call word 0x%llx\n", value1);
        }
        break;
    case 0xe9:
        if (override & cpu_override_dword_operand)
        {
            value1 = cpu_rel32(cpu_reg_ip);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("jmp dword 0x%llx\n", value1);
        }
        else
        {
            value1 = cpu_rel16(cpu_reg_ip);
            cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
            if (debug)
                printf("jmp word 0x%llx\n", value1);
        }
        break;
    case 0xea:
        value1 = cpu_imm16(cpu_reg_ip, 0);
        value2 = cpu_imm16(cpu_reg_ip, 0);
        cpu_write_reg(cpu_reg_gdtr, cpu_read_reg(cpu_reg_gdtr_next));
        cpu_write_reg(cpu_reg_ip, value1);
        cpu_write_reg(cpu_reg_cs, value2);
        if (debug)
            printf("jmp 0x%llx:0x%llx\n", value2, value1);
        break;
    case 0xeb:
        value1 = cpu_rel8(cpu_reg_ip);
        cpu_exec_jmp_near(cpu_reg_ip, value1, 2);
        if (debug)
            printf("jmp 0x%llx\n", value1);
        break;
    case 0xec:
        if (debug)
            printf("in al, dx\n");
        cpu_write_reg(cpu_reg_al, io_read(cpu_read_reg(cpu_reg_dx), 1));
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    case 0xed:
        if (debug)
            printf("in ax, dx\n");
        cpu_write_reg(cpu_reg_ax, io_read(cpu_read_reg(cpu_reg_dx), 2));
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
            printf("db 0x%x\n", opcode_byte);
        cpu_add_reg(cpu_reg_ip, 1);
        break;
    }
    cpu_info_index = 0;
}
