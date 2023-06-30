#include "cpu.h"

extern SDL_Window *window;
extern SDL_Surface *window_surface;
extern uint64_t window_framebuffer[];

uint64_t cpu_state[53];
uint8_t regs32[] = {
    cpu_eax,
    cpu_ecx,
    cpu_edx,
    cpu_ebx,
    cpu_esp,
    cpu_ebp,
    cpu_esi,
    cpu_edi,
};
uint8_t regs16[] = {
    cpu_ax,
    cpu_cx,
    cpu_dx,
    cpu_bx,
    cpu_sp,
    cpu_bp,
    cpu_si,
    cpu_di,
};
uint8_t regs8[] = {
    cpu_al,
    cpu_cl,
    cpu_dl,
    cpu_bl,
    cpu_ah,
    cpu_ch,
    cpu_dh,
    cpu_bh,
};

void cpu_dump_state()
{
    printf("cpu debug : {\n");
    for (size_t i = 0; i < sizeof(cpu_state) / sizeof(uint64_t); i++)
        printf("\t%s: 0x%llx;\n", cpu_regs_string[i], (unsigned long long)cpu_state[i]);
    printf("};\n");
}

void cpu_exec_instruction(uint16_t instruction, uint64_t reg1, uint64_t reg2, uint8_t reg1_type, uint8_t reg2_type)
{
    if (reg1_type == cpu_type_int)
        return;
    if (reg1_type == cpu_type_reg || reg1_type == cpu_type_buffer_reg)
        if (reg1 > sizeof(cpu_state) / sizeof(uint64_t))
            return;
    if (reg2_type == cpu_type_reg || reg2_type == cpu_type_buffer_reg)
        if (reg2 > sizeof(cpu_state) / sizeof(uint64_t))
            return;
    uint64_t reg1_id = reg1, reg2_id = reg2;
    if (reg2_type == cpu_type_buffer_reg)
    {
        if (reg2_id >= cpu_gs && reg2_id <= cpu_dl)
            reg2 = *(uint8_t *)(vram + cpu_state[reg2]);
        else if (reg2_id >= cpu_di && reg2_id <= cpu_ip)
            reg2 = *(uint16_t *)(vram + cpu_state[reg2]);
        else if (reg2_id >= cpu_edi && reg2_id <= cpu_eip)
            reg2 = *(uint32_t *)(vram + cpu_state[reg2]);
        else if (reg2_id >= cpu_rdi && reg2_id <= cpu_r15)
            reg2 = *(uint64_t *)(vram + cpu_state[reg2]);
    }
    if (reg2_type == cpu_type_buffer)
    {
        if (reg2_id >= cpu_gs && reg2_id <= cpu_dl)
            reg2 = *(uint8_t *)(vram + reg2);
        else if (reg2_id >= cpu_di && reg2_id <= cpu_ip)
            reg2 = *(uint16_t *)(vram + reg2);
        else if (reg2_id >= cpu_edi && reg2_id <= cpu_eip)
            reg2 = *(uint32_t *)(vram + reg2);
        else if (reg2_id >= cpu_rdi && reg2_id <= cpu_r15)
            reg2 = *(uint64_t *)(vram + reg2);
    }
    if (reg1_type == cpu_type_buffer_reg)
        reg1 = cpu_state[reg1];
    switch (instruction)
    {
    case cpu_instruction_add:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(cpu_state[reg1] + (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(cpu_state[reg1] + (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(cpu_state[reg1] + (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(cpu_state[reg1] + (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) += reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) += reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) += reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) += reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_sub:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(cpu_state[reg1] - (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(cpu_state[reg1] - (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(cpu_state[reg1] - (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(cpu_state[reg1] - (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) -= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) -= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) -= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) -= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_mul:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(cpu_state[reg1] * (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(cpu_state[reg1] * (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(cpu_state[reg1] * (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(cpu_state[reg1] * (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) *= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) *= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) *= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) *= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_div:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(cpu_state[reg1] / (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(cpu_state[reg1] / (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(cpu_state[reg1] / (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(cpu_state[reg1] / (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) /= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) /= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) /= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) /= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_and:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(cpu_state[reg1] & (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(cpu_state[reg1] & (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(cpu_state[reg1] & (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(cpu_state[reg1] & (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) &= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) &= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) &= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) &= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_or:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(cpu_state[reg1] | (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(cpu_state[reg1] | (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(cpu_state[reg1] | (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(cpu_state[reg1] | (reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2));
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) |= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) |= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) |= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) |= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_mov:
        if (reg1_type == cpu_type_reg)
        {
            if (reg1_id >= cpu_gs && reg1_id <= cpu_dl)
                cpu_state[reg1] = (uint8_t)(reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2);
            else if (reg1_id >= cpu_di && reg1_id <= cpu_ip)
                cpu_state[reg1] = (uint16_t)(reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2);
            else if (reg1_id >= cpu_edi && reg1_id <= cpu_eip)
                cpu_state[reg1] = (uint32_t)(reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2);
            else if (reg1_id >= cpu_rdi && reg1_id <= cpu_r15)
                cpu_state[reg1] = (uint64_t)(reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2);
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg2_id >= cpu_gs && reg2_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg2_id >= cpu_di && reg2_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg2_id >= cpu_edi && reg2_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg2_id >= cpu_rdi && reg2_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    case cpu_instruction_push:
        if (reg1_type == cpu_type_reg)
            ;
        break;
    case cpu_instruction_pop:
        if (reg1_type == cpu_type_reg)
            ;
        break;
    default:
        break;
    }
}

uint16_t cpu_rm16(uint8_t reg)
{
    cpu_state[reg]++;
    uint8_t rm8 = vram[cpu_state[reg]] & 7;
    return regs16[rm8];
}

uint16_t cpu_rm8(uint8_t reg)
{
    cpu_state[reg]++;
    uint8_t rm8 = vram[cpu_state[reg]] & 7;
    return regs8[rm8];
}

uint16_t cpu_imm16(uint8_t reg)
{
    cpu_state[reg]++;
    uint8_t b1 = vram[cpu_state[reg]];
    cpu_state[reg]++;
    uint8_t b2 = vram[cpu_state[reg]];
    uint16_t imm = (b2 << 8) | b1;
    return (uint16_t)imm;
}

uint8_t cpu_imm8(uint8_t reg)
{
    cpu_state[reg]++;
    uint8_t imm = vram[cpu_state[reg]];
    return (uint8_t)imm;
}

uint16_t cpu_rel16(uint8_t reg)
{
    cpu_state[reg]++;
    uint8_t b1 = vram[cpu_state[reg]];
    cpu_state[reg]++;
    uint8_t b2 = vram[cpu_state[reg]];
    uint16_t rel = (b2 << 8) | b1;
    return (uint16_t)(cpu_state[reg] + rel + 1);
}

uint8_t cpu_rel8(uint8_t reg)
{
    cpu_state[reg]++;
    uint8_t rel = vram[cpu_state[reg]];
    return (uint8_t)(cpu_state[reg] + rel + 1);
}

void cpu_emulate_i8086(uint8_t debug)
{
    uint8_t *opcode = &vram[cpu_state[cpu_ip]];
    uint64_t reg_id;
    uint64_t value;
    switch (*opcode)
    {
    case 0x00:
        cpu_state[cpu_ip]++;
        break;
    case 0x80:
        switch ((opcode[1] & 0x38) >> 3)
        {
        case 0x00:
            reg_id = cpu_rm8(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_exec_instruction(cpu_instruction_add,
                                 reg_id, value,
                                 cpu_type_reg, cpu_type_int);
            if (debug)
                printf("add %s, %llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        case 0x05:
            reg_id = cpu_rm8(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_exec_instruction(cpu_instruction_sub,
                                 reg_id, value,
                                 cpu_type_reg, cpu_type_int);
            if (debug)
                printf("sub %s, %llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        default:
            break;
        }
        cpu_state[cpu_ip]++;
        break;
    case 0x81:
        switch ((opcode[1] & 0x38) >> 3)
        {
        case 0x00:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm16(cpu_ip);
            cpu_exec_instruction(cpu_instruction_add,
                                 reg_id, value,
                                 cpu_type_reg, cpu_type_int);
            if (debug)
                printf("add %s, %llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        case 0x05:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm16(cpu_ip);
            cpu_exec_instruction(cpu_instruction_sub,
                                 reg_id, value,
                                 cpu_type_reg, cpu_type_int);
            if (debug)
                printf("sub %s, %llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        default:
            break;
        }
        cpu_state[cpu_ip]++;
        break;
    case 0x83:
        switch ((opcode[1] & 0x38) >> 3)
        {
        case 0x00:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_exec_instruction(cpu_instruction_add,
                                 reg_id, value,
                                 cpu_type_reg, cpu_type_int);
            if (debug)
                printf("add %s, %llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        case 0x05:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_exec_instruction(cpu_instruction_sub,
                                 reg_id, value,
                                 cpu_type_reg, cpu_type_int);
            if (debug)
                printf("sub %s, %llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        default:
            break;
        }
        cpu_state[cpu_ip]++;
        break;
    case 0x90:
        cpu_state[cpu_ip]++;
        if (debug)
            printf("nop\n");
        break;
    case 0xb0:
        cpu_exec_instruction(cpu_instruction_mov, cpu_al, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov al, 0x%llx\n", (unsigned long long)cpu_state[cpu_al]);
        break;
    case 0xb1:
        cpu_exec_instruction(cpu_instruction_mov, cpu_cl, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov cl, 0x%llx\n", (unsigned long long)cpu_state[cpu_cl]);
        break;
    case 0xb2:
        cpu_exec_instruction(cpu_instruction_mov, cpu_dl, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov dl, 0x%llx\n", (unsigned long long)cpu_state[cpu_dl]);
        break;
    case 0xb3:
        cpu_exec_instruction(cpu_instruction_mov, cpu_bl, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov bl, 0x%llx\n", (unsigned long long)cpu_state[cpu_bl]);
        break;
    case 0xb4:
        cpu_exec_instruction(cpu_instruction_mov, cpu_ah, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov ah, 0x%llx\n", (unsigned long long)cpu_state[cpu_ah]);
        break;
    case 0xb5:
        cpu_exec_instruction(cpu_instruction_mov, cpu_ch, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov ch, 0x%llx\n", (unsigned long long)cpu_state[cpu_ch]);
        break;
    case 0xb6:
        cpu_exec_instruction(cpu_instruction_mov, cpu_dh, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov dh, 0x%llx\n", (unsigned long long)cpu_state[cpu_dh]);
        break;
    case 0xb7:
        cpu_exec_instruction(cpu_instruction_mov, cpu_bh, cpu_imm8(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov bh, 0x%llx\n", (unsigned long long)cpu_state[cpu_bh]);
        break;
    case 0xb8:
        cpu_exec_instruction(cpu_instruction_mov, cpu_ax, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov ax, 0x%llx\n", (unsigned long long)cpu_state[cpu_ax]);
        break;
    case 0xb9:
        cpu_exec_instruction(cpu_instruction_mov, cpu_cx, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov cx, 0x%llx\n", (unsigned long long)cpu_state[cpu_cx]);
        break;
    case 0xba:
        cpu_exec_instruction(cpu_instruction_mov, cpu_dx, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov dx, 0x%llx\n", (unsigned long long)cpu_state[cpu_dx]);
        break;
    case 0xbb:
        cpu_exec_instruction(cpu_instruction_mov, cpu_bx, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov bx, 0x%llx\n", (unsigned long long)cpu_state[cpu_bx]);
        break;
    case 0xbc:
        cpu_exec_instruction(cpu_instruction_mov, cpu_sp, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov sp, 0x%llx\n", (unsigned long long)cpu_state[cpu_sp]);
        break;
    case 0xbd:
        cpu_exec_instruction(cpu_instruction_mov, cpu_bp, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov bp, 0x%llx\n", (unsigned long long)cpu_state[cpu_bp]);
        break;
    case 0xbe:
        cpu_exec_instruction(cpu_instruction_mov, cpu_si, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov si, 0x%llx\n", (unsigned long long)cpu_state[cpu_si]);
        break;
    case 0xbf:
        cpu_exec_instruction(cpu_instruction_mov, cpu_di, cpu_imm16(cpu_ip), cpu_type_reg, cpu_type_int);
        if (debug)
            printf("mov di, 0x%llx\n", (unsigned long long)cpu_state[cpu_di]);
        break;
    case 0xe9:
        cpu_state[cpu_ip] = cpu_rel16(cpu_ip);
        if (debug)
            printf("jmp 0x%llx\n", (unsigned long long)cpu_state[cpu_ip]);
    case 0xeb:
        cpu_state[cpu_ip] = cpu_rel8(cpu_ip);
        if (debug)
            printf("jmp 0x%llx\n", (unsigned long long)cpu_state[cpu_ip]);
        break;
    case 0xec:
        if (debug)
            printf("in al, dx\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xed:
        if (debug)
            printf("in ax, dx\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xee:
        if (debug)
            printf("out dx, al\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xef:
        if (debug)
            printf("out dx, ax\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xcd:
        value = cpu_imm8(cpu_ip);
        if (value == 0x20)
            for (size_t i = 0; i < window_framebuffer[1] * window_framebuffer[2] * window_framebuffer[3]; i++)
                vram[window_framebuffer[0] + i] = (uint8_t)cpu_state[cpu_ax];
        if (debug)
            printf("int 0x%llx\n", (unsigned long long)value);
        cpu_state[cpu_ip]++;
        break;
    case 0xf0:
        if (debug)
            printf("lock\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xf2:
        if (debug)
            printf("repne\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xf3:
        if (debug)
            printf("rep\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xf4:
        if (debug)
            printf("hlt\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xf5:
        if (debug)
            printf("cmc\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xf8:
        if (debug)
            printf("clc\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xf9:
        if (debug)
            printf("stc\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xfa:
        if (debug)
            printf("cli\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xfb:
        if (debug)
            printf("sti\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xfc:
        if (debug)
            printf("cld\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xfd:
        if (debug)
            printf("std\n");
        cpu_state[cpu_ip]++;
        break;
    default:
        if (debug)
            printf("db 0x%hhx\n", *opcode);
        cpu_state[cpu_ip]++;
        break;
    }
}
