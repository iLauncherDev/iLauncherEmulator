#include "cpu.h"

extern SDL_Window *window;
extern SDL_Surface *window_surface;
extern uint64_t window_framebuffer[];

uint8_t x80_precalc[256];
uint8_t rm_offset[] = {
    0,
    0,
    0,
    0,
    cpu_reg_edi - cpu_reg_di,
    0,
    0,
    0,
    cpu_reg_rdi - cpu_reg_di,
};
cpu_info_t cpu_info[2];
uint8_t cpu_info_index = 0;
uint8_t cpu_resetting = false;

uint8_t *opcode;
uint64_t cpu_state[53];
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

void cpu_reset()
{
    uint8_t i = sizeof(cpu_state) / sizeof(uint64_t);
    while (i--)
        cpu_state[i] = 0;
    cpu_resetting = true;
}

void cpu_setup_precalcs()
{
    for (size_t i = 0; i < 0xff; i++)
        x80_precalc[i] = (i & 0x38) >> 3;
}

void cpu_dump_state()
{
    printf("cpu debug : {\n");
    for (size_t i = 0; i < sizeof(cpu_state) / sizeof(uint64_t); i++)
        printf("\t%s: 0x%llx;\n", cpu_regs_string[i], (unsigned long long)cpu_state[i]);
    printf("};\n");
}

static inline uint64_t cpu_rm(uint8_t reg, uint8_t size)
{
    opcode++, cpu_state[reg]++;
    uint8_t mod = *opcode >> 6;
    uint8_t rm8 = *opcode & 7;
    switch (mod)
    {
    case 0x00:
        if (rm8 == 0x06)
        {
            opcode++, cpu_state[reg]++;
            uint8_t b1 = *opcode;
            opcode++, cpu_state[reg]++;
            uint8_t b2 = *opcode;
            uint16_t value = (b2 << 8) | b1;
            cpu_info[cpu_info_index].reg_type = cpu_type_memory;
            cpu_info_index = (cpu_info_index + 1) & 1;
            return value;
        }
        break;
    case 0x03:
        switch (size)
        {
        case 1:
            cpu_info[cpu_info_index].reg_type = cpu_type_reg;
            cpu_info_index = (cpu_info_index + 1) & 1;
            return regs8[rm8];
        case 2:
            cpu_info[cpu_info_index].reg_type = cpu_type_reg;
            cpu_info_index = (cpu_info_index + 1) & 1;
            return regs16[rm8];
        }
        return 0x00;
    }
    switch (rm8)
    {
    case 0x00:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx + rm_offset[size];
        cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_si + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x01:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx + rm_offset[size];
        cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_di + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x02:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp + rm_offset[size];
        cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_si + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x03:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 2;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp + rm_offset[size];
        cpu_info[cpu_info_index].reg_type_buffer[2] = cpu_reg_di + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x04:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_si + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x05:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_di + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x06:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bp + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    case 0x07:
        cpu_info[cpu_info_index].reg_type = cpu_type_memory_reg;
        cpu_info[cpu_info_index].reg_type_buffer[0] = 1;
        cpu_info[cpu_info_index].reg_type_buffer[1] = cpu_reg_bx + rm_offset[size];
        cpu_info_index = (cpu_info_index + 1) & 1;
        break;
    }
    return 0xff;
}

static inline uint8_t cpu_r16(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    cpu_info[cpu_info_index].reg_type = cpu_type_reg;
    cpu_info_index = (cpu_info_index + 1) & 1;
    return regs16[(*opcode & 0x38) >> 3];
}

static inline uint8_t cpu_r8(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    cpu_info[cpu_info_index].reg_type = cpu_type_reg;
    cpu_info_index = (cpu_info_index + 1) & 1;
    return regs8[(*opcode & 0x38) >> 3];
}

static inline uint16_t cpu_rm16(uint8_t reg)
{
    return (uint16_t)cpu_rm(reg, 2);
}

static inline uint8_t cpu_rm8(uint8_t reg)
{
    return (uint8_t)cpu_rm(reg, 1);
}

static inline uint16_t cpu_imm16(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t b1 = *opcode;
    opcode++, cpu_state[reg]++;
    uint8_t b2 = *opcode;
    uint16_t imm = (b2 << 8) | b1;
    cpu_info[cpu_info_index].reg_type = cpu_type_int;
    cpu_info_index = (cpu_info_index + 1) & 1;
    return (uint16_t)imm;
}

static inline uint8_t cpu_imm8(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t imm = *opcode;
    cpu_info[cpu_info_index].reg_type = cpu_type_int;
    cpu_info_index = (cpu_info_index + 1) & 1;
    return (uint8_t)imm;
}

static inline uint16_t cpu_rel16(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t b1 = *opcode;
    opcode++, cpu_state[reg]++;
    uint8_t b2 = *opcode;
    uint16_t rel = (b2 << 8) | b1;
    cpu_info[cpu_info_index].reg_type = cpu_type_int;
    cpu_info_index = (cpu_info_index + 1) & 1;
    return (uint16_t)(cpu_state[reg] + rel + 1);
}

static inline uint8_t cpu_rel8(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t rel = *opcode;
    cpu_info[cpu_info_index].reg_type = cpu_type_int;
    cpu_info_index = (cpu_info_index + 1) & 1;
    return (uint8_t)(cpu_state[reg] + rel + 1);
}

static inline uint64_t cpu_resolve_value(uint64_t value, uint8_t index, uint8_t size)
{
    switch (cpu_info[index].reg_type)
    {
    case cpu_type_memory:
        return memory_read(&vm_memory[value], size);
    case cpu_type_memory_reg:
        return memory_read(&vm_memory[value], size);
    case cpu_type_reg:
        return cpu_state[value];
    };
    return value;
}

static inline void cpu_exec_mov(uint64_t value1, uint64_t value2, uint8_t size)
{
    if (cpu_info[0].reg_type == cpu_type_memory_reg)
        value1 = cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                     : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                        (cpu_state[cpu_info[0].reg_type_buffer[2]]));
    if (cpu_info[1].reg_type == cpu_type_memory_reg)
        value2 = cpu_info[1].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[1].reg_type_buffer[1]])
                                                     : (cpu_state[cpu_info[1].reg_type_buffer[1]] +
                                                        (cpu_state[cpu_info[1].reg_type_buffer[2]]));
    value2 = cpu_resolve_value(value2, 1, size);
    switch (cpu_info[0].reg_type)
    {
    case cpu_type_memory:
        memory_write(&vm_memory[value1], value2, size);
        break;
    case cpu_type_memory_reg:
        memory_write(&vm_memory[value1], value2, size);
        break;
    default:
        memory_write(&cpu_state[value1], value2, size);
        break;
    };
}

static inline void cpu_push_reg(uint8_t stack, uint8_t reg, uint8_t size)
{
    if ((int64_t)cpu_state[stack] - size <= 0)
        cpu_reset();
    cpu_state[stack] -= size;
    switch (size)
    {
    case 1:
        *((uint8_t *)&vm_memory[cpu_state[stack]]) = cpu_state[reg];
        break;
    case 2:
        *((uint16_t *)&vm_memory[cpu_state[stack]]) = cpu_state[reg];
        break;
    case 4:
        *((uint32_t *)&vm_memory[cpu_state[stack]]) = cpu_state[reg];
        break;
    default:
        *((uint64_t *)&vm_memory[cpu_state[stack]]) = cpu_state[reg];
        break;
    }
}

static inline void cpu_pop_reg(uint8_t stack, uint8_t reg, uint8_t size)
{
    if ((int64_t)cpu_state[stack] + size >= vm_memory_size)
        cpu_reset();
    switch (size)
    {
    case 1:
        cpu_state[reg] = *((uint8_t *)&vm_memory[cpu_state[stack]]);
        break;
    case 2:
        cpu_state[reg] = *((uint16_t *)&vm_memory[cpu_state[stack]]);
        break;
    case 4:
        cpu_state[reg] = *((uint32_t *)&vm_memory[cpu_state[stack]]);
        break;
    default:
        cpu_state[reg] = *((uint64_t *)&vm_memory[cpu_state[stack]]);
        break;
    }
    cpu_state[stack] += size;
}

void cpu_emulate_i8086(uint8_t debug)
{
    cpu_state[cpu_reg_ip] &= 0xffff;
    opcode = &vm_memory[cpu_state[cpu_reg_ip]];
    uint16_t value1;
    uint16_t value2;
    char *operation;
    if (debug)
        printf("IP: 0x%x, OPCODE 0x%x: ", (uint16_t)cpu_state[cpu_reg_ip], *opcode);
    switch (*opcode)
    {
    case 0x06:
        cpu_push_reg(cpu_reg_sp, cpu_reg_es, 2);
        if (debug)
            printf("push word es\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x07:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_es, 2);
        if (debug)
            printf("pop word es\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x0e:
        cpu_push_reg(cpu_reg_sp, cpu_reg_cs, 2);
        if (debug)
            printf("push word cs\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x0f:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_cs, 2);
        if (debug)
            printf("pop word cs\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x16:
        cpu_push_reg(cpu_reg_sp, cpu_reg_ss, 2);
        if (debug)
            printf("push word ss\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x17:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_ss, 2);
        if (debug)
            printf("pop word ss\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x1e:
        cpu_push_reg(cpu_reg_sp, cpu_reg_ds, 2);
        if (debug)
            printf("push word ds\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x1f:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_ds, 2);
        if (debug)
            printf("pop word ds\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x50:
        cpu_push_reg(cpu_reg_sp, cpu_reg_ax, 2);
        if (debug)
            printf("push word ax\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x51:
        cpu_push_reg(cpu_reg_sp, cpu_reg_cx, 2);
        if (debug)
            printf("push word cx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x52:
        cpu_push_reg(cpu_reg_sp, cpu_reg_dx, 2);
        if (debug)
            printf("push word dx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x53:
        cpu_push_reg(cpu_reg_sp, cpu_reg_bx, 2);
        if (debug)
            printf("push word bx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x54:
        cpu_push_reg(cpu_reg_sp, cpu_reg_sp, 2);
        if (debug)
            printf("push word sp\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x55:
        cpu_push_reg(cpu_reg_sp, cpu_reg_bp, 2);
        if (debug)
            printf("push word bp\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x56:
        cpu_push_reg(cpu_reg_sp, cpu_reg_si, 2);
        if (debug)
            printf("push word si\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x57:
        cpu_push_reg(cpu_reg_sp, cpu_reg_di, 2);
        if (debug)
            printf("push word di\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x58:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_ax, 2);
        if (debug)
            printf("pop word ax\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x59:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_cx, 2);
        if (debug)
            printf("pop word cx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x5a:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_dx, 2);
        if (debug)
            printf("pop word dx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x5b:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_bx, 2);
        if (debug)
            printf("pop word bx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x5c:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_sp, 2);
        if (debug)
            printf("pop word sp\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x5d:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_bp, 2);
        if (debug)
            printf("pop word bp\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x5e:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_si, 2);
        if (debug)
            printf("pop word si\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x5f:
        cpu_pop_reg(cpu_reg_sp, cpu_reg_di, 2);
        if (debug)
            printf("pop word di\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x60:
        for (uint8_t i = cpu_reg_ip; i > cpu_reg_di; i--)
            cpu_push_reg(cpu_reg_sp, i, 2);
        if (debug)
            printf("pusha\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x61:
        for (uint8_t i = cpu_reg_ip; i > cpu_reg_di; i--)
            cpu_pop_reg(cpu_reg_sp, cpu_reg_ip - i, 2);
        if (debug)
            printf("popa\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x6a:
        value2 = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("push byte 0x%x\n", value2);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x68:
        value2 = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("push word 0x%x\n", value2);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x80:
        switch (x80_precalc[opcode[1]])
        {
        case 0x00:
            operation = "add";
            value1 = cpu_rm8(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint8_t)(vm_memory[value1] + value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint8_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                1
                                            ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                            : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                               (cpu_state[cpu_info[0].reg_type_buffer[2]]))] +
                              value2);
            else
                cpu_state[value1] = (uint8_t)(cpu_state[value1] + value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s byte [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s byte [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s byte [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s byte %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x01:
            operation = "or";
            value1 = cpu_rm8(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint8_t)(vm_memory[value1] | value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint8_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                1
                                            ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                            : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                               (cpu_state[cpu_info[0].reg_type_buffer[2]]))] |
                              value2);
            else
                cpu_state[value1] = (uint8_t)(cpu_state[value1] | value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s byte [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s byte [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s byte [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s byte %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x04:
            operation = "and";
            value1 = cpu_rm8(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint8_t)(vm_memory[value1] & value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint8_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                1
                                            ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                            : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                               (cpu_state[cpu_info[0].reg_type_buffer[2]]))] &
                              value2);
            else
                cpu_state[value1] = (uint8_t)(cpu_state[value1] & value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s byte [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s byte [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s byte [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s byte %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x05:
            operation = "sub";
            value1 = cpu_rm8(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint8_t)(vm_memory[value1] - value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint8_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                1
                                            ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                            : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                               (cpu_state[cpu_info[0].reg_type_buffer[2]]))] -
                              value2);
            else
                cpu_state[value1] = (uint8_t)(cpu_state[value1] - value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s byte [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s byte [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s byte [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s byte %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        default:
            goto unknown;
            break;
        }
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x81:
        switch (x80_precalc[opcode[1]])
        {
        case 0x00:
            operation = "add";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm16(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] + value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] +
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] + value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x01:
            operation = "or";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm16(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] | value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] |
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] | value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x04:
            operation = "and";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm16(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] & value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] &
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] & value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x05:
            operation = "sub";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm16(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] - value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] -
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] - value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        default:
            goto unknown;
            break;
        }
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x83:
        switch (x80_precalc[opcode[1]])
        {
        case 0x00:
            operation = "add";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] + value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] +
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] + value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x01:
            operation = "or";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] | value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] |
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] | value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x04:
            operation = "and";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] & value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] &
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] & value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        case 0x05:
            operation = "sub";
            value1 = cpu_rm16(cpu_reg_ip);
            value2 = cpu_imm8(cpu_reg_ip);
            if (cpu_info[0].reg_type == cpu_type_memory)
                vm_memory[value1] = (uint16_t)(vm_memory[value1] - value2);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                vm_memory[cpu_info[0].reg_type_buffer[0] == 1 ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                                              : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                                 (cpu_state[cpu_info[0].reg_type_buffer[2]]))] =
                    (uint16_t)(vm_memory[cpu_info[0].reg_type_buffer[0] ==
                                                 1
                                             ? (cpu_state[cpu_info[0].reg_type_buffer[1]])
                                             : (cpu_state[cpu_info[0].reg_type_buffer[1]] +
                                                (cpu_state[cpu_info[0].reg_type_buffer[2]]))] -
                               value2);
            else
                cpu_state[value1] = (uint16_t)(cpu_state[value1] - value2);
            if (debug)
            {
                if (cpu_info[0].reg_type == cpu_type_memory)
                    printf("%s word [0x%x], 0x%llx\n", operation, value1, (unsigned long long)value2);
                else if (cpu_info[0].reg_type == cpu_type_memory_reg)
                    cpu_info[0].reg_type_buffer[0] == 1 ? printf("%s word [%s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 (unsigned long long)value2)
                                                        : printf("%s word [%s + %s], 0x%llx\n",
                                                                 operation,
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                                                                 cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                                                                 (unsigned long long)value2);
                else
                    printf("%s word %s, 0x%llx\n", operation, cpu_regs_string[value1], (unsigned long long)value2);
            }
            break;
        default:
            goto unknown;
            break;
        }
        cpu_state[cpu_reg_ip]++;
        break;
    case 0x88:
        value1 = cpu_rm8(cpu_reg_ip);
        opcode--, cpu_state[cpu_reg_ip]--;
        value2 = cpu_r8(cpu_reg_ip);
        opcode++, cpu_state[cpu_reg_ip]++;
        cpu_exec_mov(value1, value2, 1);
        if (debug)
        {
            if (cpu_info[0].reg_type == cpu_type_memory)
                printf("mov byte [0x%x], %s\n", value1, cpu_regs_string[value2]);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
            {
                if (cpu_info[0].reg_type_buffer[0] == 1)
                    printf("mov byte [%s], %s\n",
                           cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                           cpu_regs_string[value2]);
                else if (cpu_info[0].reg_type_buffer[0] == 2)
                    printf("mov byte [%s + %s], %s\n",
                           cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                           cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                           cpu_regs_string[value2]);
            }
            else
                printf("mov byte %s, %s\n", cpu_regs_string[value1], cpu_regs_string[value2]);
        }
        break;
    case 0x89:
        value1 = cpu_rm16(cpu_reg_ip);
        opcode--, cpu_state[cpu_reg_ip]--;
        value2 = cpu_r16(cpu_reg_ip);
        opcode++, cpu_state[cpu_reg_ip]++;
        cpu_exec_mov(value1, value2, 2);
        if (debug)
        {
            if (cpu_info[0].reg_type == cpu_type_memory)
                printf("mov word [0x%x], %s\n", value1, cpu_regs_string[value2]);
            else if (cpu_info[0].reg_type == cpu_type_memory_reg)
            {
                if (cpu_info[0].reg_type_buffer[0] == 1)
                    printf("mov word [%s], %s\n",
                           cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                           cpu_regs_string[value2]);
                else if (cpu_info[0].reg_type_buffer[0] == 2)
                    printf("mov word [%s + %s], %s\n",
                           cpu_regs_string[cpu_info[0].reg_type_buffer[1]],
                           cpu_regs_string[cpu_info[0].reg_type_buffer[2]],
                           cpu_regs_string[value2]);
            }
            else
                printf("mov word %s, %s\n", cpu_regs_string[value1], cpu_regs_string[value2]);
        }
        break;
    case 0x8a:
        value1 = cpu_r8(cpu_reg_ip);
        opcode--, cpu_state[cpu_reg_ip]--;
        value2 = cpu_rm8(cpu_reg_ip);
        opcode++, cpu_state[cpu_reg_ip]++;
        cpu_exec_mov(value1, value2, 1);
        if (debug)
        {
            if (cpu_info[1].reg_type == cpu_type_memory)
                printf("mov byte %s, [0x%x]\n", cpu_regs_string[value1], value2);
            else if (cpu_info[1].reg_type == cpu_type_memory_reg)
            {
                if (cpu_info[1].reg_type_buffer[0] == 1)
                    printf("mov byte %s, [%s]\n",
                           cpu_regs_string[value1],
                           cpu_regs_string[cpu_info[1].reg_type_buffer[1]]);
                else if (cpu_info[1].reg_type_buffer[0] == 2)
                    printf("mov byte %s, [%s + %s]\n",
                           cpu_regs_string[value1],
                           cpu_regs_string[cpu_info[1].reg_type_buffer[1]],
                           cpu_regs_string[cpu_info[1].reg_type_buffer[2]]);
            }
            else
                printf("mov byte %s, %s\n", cpu_regs_string[value1], cpu_regs_string[value2]);
        }
        break;
    case 0x8b:
        value1 = cpu_r16(cpu_reg_ip);
        opcode--, cpu_state[cpu_reg_ip]--;
        value2 = cpu_rm16(cpu_reg_ip);
        opcode++, cpu_state[cpu_reg_ip]++;
        cpu_exec_mov(value1, value2, 2);
        if (debug)
        {
            if (cpu_info[1].reg_type == cpu_type_memory)
                printf("mov word %s, [0x%x]\n", cpu_regs_string[value1], value2);
            else if (cpu_info[1].reg_type == cpu_type_memory_reg)
            {
                if (cpu_info[1].reg_type_buffer[0] == 1)
                    printf("mov word %s, [%s]\n",
                           cpu_regs_string[value1],
                           cpu_regs_string[cpu_info[1].reg_type_buffer[1]]);
                else if (cpu_info[1].reg_type_buffer[0] == 2)
                    printf("mov word %s, [%s + %s]\n",
                           cpu_regs_string[value1],
                           cpu_regs_string[cpu_info[1].reg_type_buffer[1]],
                           cpu_regs_string[cpu_info[1].reg_type_buffer[2]]);
            }
            else
                printf("mov word %s, %s\n", cpu_regs_string[value1], cpu_regs_string[value2]);
        }
        break;
    case 0x90:
        if (debug)
            printf("nop\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb0:
        cpu_state[cpu_reg_al] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte al, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_al]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb1:
        cpu_state[cpu_reg_cl] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte cl, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_cl]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb2:
        cpu_state[cpu_reg_dl] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte dl, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_dl]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb3:
        cpu_state[cpu_reg_bl] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte bl, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_bl]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb4:
        cpu_state[cpu_reg_ah] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte ah, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_ah]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb5:
        cpu_state[cpu_reg_ch] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte ch, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_ch]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb6:
        cpu_state[cpu_reg_dh] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte dh, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_dh]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb7:
        cpu_state[cpu_reg_bh] = cpu_imm8(cpu_reg_ip);
        if (debug)
            printf("mov byte bh, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_bh]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb8:
        cpu_state[cpu_reg_ax] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word ax, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_ax]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xb9:
        cpu_state[cpu_reg_cx] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word cx, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_cx]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xba:
        cpu_state[cpu_reg_dx] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word dx, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_dx]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xbb:
        cpu_state[cpu_reg_bx] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word bx, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_bx]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xbc:
        cpu_state[cpu_reg_sp] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word sp, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_sp]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xbd:
        cpu_state[cpu_reg_bp] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word bp, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_bp]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xbe:
        cpu_state[cpu_reg_si] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word si, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_si]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xbf:
        cpu_state[cpu_reg_di] = cpu_imm16(cpu_reg_ip);
        if (debug)
            printf("mov word di, 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_di]);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xcd:
        value2 = cpu_imm8(cpu_reg_ip);
        if (value2 == 0xff)
        {
            exit(0);
        }
        else if (value2 == 0xfe)
        {
            cpu_state[cpu_reg_ip] = 0;
            break;
        }
        else if (value2 == 0x20)
        {
            for (uint32_t i = window_framebuffer[0];
                 i < window_framebuffer[0] +
                         window_framebuffer[1] * window_framebuffer[2] * window_framebuffer[3];
                 i++)
                vm_memory[i] = cpu_state[cpu_reg_ax];
        }
        if (debug)
            printf("int byte 0x%llx\n", (unsigned long long)value2);
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xe9:
        cpu_state[cpu_reg_ip] = cpu_rel16(cpu_reg_ip);
        if (debug)
            printf("jmp word 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_ip]);
        break;
    case 0xeb:
        cpu_state[cpu_reg_ip] = cpu_rel8(cpu_reg_ip);
        if (debug)
            printf("jmp byte 0x%llx\n", (unsigned long long)cpu_state[cpu_reg_ip]);
        break;
    case 0xec:
        if (debug)
            printf("in al, dx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xed:
        if (debug)
            printf("in  ax, dx\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xee:
        if (debug)
            printf("out dx, al\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xef:
        if (debug)
            printf("out dx, ax\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf0:
        if (debug)
            printf("lock\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf2:
        if (debug)
            printf("repne\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf3:
        if (debug)
            printf("rep\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf4:
        if (debug)
            printf("hlt\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf5:
        if (debug)
            printf("cmc\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf8:
        if (debug)
            printf("clc\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xf9:
        if (debug)
            printf("stc\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xfa:
        if (debug)
            printf("cli\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xfb:
        if (debug)
            printf("sti\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xfc:
        if (debug)
            printf("cld\n");
        cpu_state[cpu_reg_ip]++;
        break;
    case 0xfd:
        if (debug)
            printf("std\n");
        cpu_state[cpu_reg_ip]++;
        break;
    unknown:
    default:
        if (debug)
            printf("db 0x%hhx\n", *opcode);
        cpu_state[cpu_reg_ip]++;
        break;
    }
    if (cpu_resetting)
        cpu_reset(), cpu_resetting = false;
    cpu_info_index = 0;
}
