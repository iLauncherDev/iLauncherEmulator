#include "cpu.h"

uint64_t cpu_state[51];

void cpu_dump_state()
{
    printf("cpu debug : {\n");
    for (size_t i = 0; i < sizeof(cpu_state) / sizeof(uint64_t); i++)
        printf("\t%s: 0x%lx;\n", cpu_regs_string[i], cpu_state[i]);
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
        if (reg2_id >= cpu_ah && reg2_id <= cpu_dl)
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
        if (reg2_id >= cpu_ah && reg2_id <= cpu_dl)
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
            cpu_state[reg1] += reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_ah && reg1_id <= cpu_dl)
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
            cpu_state[reg1] -= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_ah && reg1_id <= cpu_dl)
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
            cpu_state[reg1] *= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_ah && reg1_id <= cpu_dl)
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
            cpu_state[reg1] /= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_ah && reg1_id <= cpu_dl)
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
            cpu_state[reg1] &= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_ah && reg1_id <= cpu_dl)
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
            cpu_state[reg1] |= reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg1_id >= cpu_ah && reg1_id <= cpu_dl)
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
            cpu_state[reg1] = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        if (reg1_type >= cpu_type_buffer_reg && reg1_type <= cpu_type_buffer)
        {
            if (reg2_id >= cpu_ah && reg2_id <= cpu_dl)
                *(uint8_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg2_id >= cpu_di && reg2_id <= cpu_ip)
                *(uint16_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg2_id >= cpu_edi && reg2_id <= cpu_eip)
                *(uint32_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
            else if (reg2_id >= cpu_rdi && reg2_id <= cpu_r15)
                *(uint64_t *)(vram + reg1) = reg2_type == cpu_type_reg ? cpu_state[reg2] : reg2;
        }
        break;
    default:
        break;
    }
}

void cpu_emulate_i386()
{
    if (cpu_state[cpu_ip])
        cpu_state[cpu_eip] = cpu_state[cpu_ip];
    uint8_t *opcode = &vram[cpu_state[cpu_eip]];
    switch (*opcode)
    {
    case 0xeb:
        cpu_state[cpu_eip] = opcode[1];
        break;
    default:
        printf("OPCODE: 0x%x\n", *opcode);
        cpu_state[cpu_eip]++;
        extern void sleep(int);
        sleep(1);
        break;
    }
}