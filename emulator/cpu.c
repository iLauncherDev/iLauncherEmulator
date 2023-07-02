#include "cpu.h"

extern SDL_Window *window;
extern SDL_Surface *window_surface;
extern uint64_t window_framebuffer[];

uint8_t x80_precalc[256];

uint8_t *opcode;
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

static inline uint8_t cpu_rm16(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t rm8 = *opcode & 7;
    return regs16[rm8];
}

static inline uint8_t cpu_rm8(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t rm8 = *opcode & 7;
    return regs8[rm8];
}

static inline uint16_t cpu_imm16(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t b1 = *opcode;
    opcode++, cpu_state[reg]++;
    uint8_t b2 = *opcode;
    uint16_t imm = (b2 << 8) | b1;
    return (uint16_t)imm;
}

static inline uint8_t cpu_imm8(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t imm = *opcode;
    return (uint8_t)imm;
}

static inline uint16_t cpu_rel16(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t b1 = *opcode;
    opcode++, cpu_state[reg]++;
    uint8_t b2 = *opcode;
    uint16_t rel = (b2 << 8) | b1;
    return (uint16_t)(cpu_state[reg] + rel + 1);
}

static inline uint8_t cpu_rel8(uint8_t reg)
{
    opcode++, cpu_state[reg]++;
    uint8_t rel = *opcode;
    return (uint8_t)(cpu_state[reg] + rel + 1);
}

void cpu_emulate_i8086(uint8_t debug)
{
    opcode = &vm_memory[cpu_state[cpu_ip] & 0xffff];
    uint8_t reg_id;
    uint16_t value;
    switch (*opcode)
    {
    case 0x80:
        switch (x80_precalc[opcode[1]])
        {
        case 0x00:
            reg_id = cpu_rm8(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_state[reg_id] = (uint8_t)(cpu_state[reg_id] + value);
            if (debug)
                printf("add %s, 0x%llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        case 0x05:
            reg_id = cpu_rm8(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_state[reg_id] = (uint8_t)(cpu_state[reg_id] - value);
            if (debug)
                printf("sub %s, 0x%llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        default:
            goto unknown;
            break;
        }
        cpu_state[cpu_ip]++;
        break;
    case 0x81:
        switch (x80_precalc[opcode[1]])
        {
        case 0x00:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm16(cpu_ip);
            cpu_state[reg_id] = (uint16_t)(cpu_state[reg_id] + value);
            if (debug)
                printf("add %s, 0x%llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        case 0x05:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm16(cpu_ip);
            cpu_state[reg_id] = (uint16_t)(cpu_state[reg_id] - value);
            if (debug)
                printf("sub %s, 0x%llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        default:
            goto unknown;
            break;
        }
        cpu_state[cpu_ip]++;
        break;
    case 0x83:
        switch (x80_precalc[opcode[1]])
        {
        case 0x00:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_state[reg_id] = (uint32_t)(cpu_state[reg_id] + value);
            if (debug)
                printf("add %s, 0x%llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        case 0x05:
            reg_id = cpu_rm16(cpu_ip);
            value = cpu_imm8(cpu_ip);
            cpu_state[reg_id] = (uint16_t)(cpu_state[reg_id] - value);
            if (debug)
                printf("sub %s, 0x%llx\n", cpu_regs_string[reg_id], (unsigned long long)value);
            break;
        default:
            goto unknown;
            break;
        }
        cpu_state[cpu_ip]++;
        break;
    case 0x90:
        if (debug)
            printf("nop\n");
        cpu_state[cpu_ip]++;
        break;
    case 0xb0:
        cpu_state[cpu_al] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov al, 0x%llx\n", (unsigned long long)cpu_state[cpu_al]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb1:
        cpu_state[cpu_cl] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov cl, 0x%llx\n", (unsigned long long)cpu_state[cpu_cl]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb2:
        cpu_state[cpu_dl] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov dl, 0x%llx\n", (unsigned long long)cpu_state[cpu_dl]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb3:
        cpu_state[cpu_bl] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov bl, 0x%llx\n", (unsigned long long)cpu_state[cpu_bl]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb4:
        cpu_state[cpu_ah] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov ah, 0x%llx\n", (unsigned long long)cpu_state[cpu_ah]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb5:
        cpu_state[cpu_ch] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov ch, 0x%llx\n", (unsigned long long)cpu_state[cpu_ch]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb6:
        cpu_state[cpu_dh] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov dh, 0x%llx\n", (unsigned long long)cpu_state[cpu_dh]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb7:
        cpu_state[cpu_bh] = cpu_imm8(cpu_ip);
        if (debug)
            printf("mov bh, 0x%llx\n", (unsigned long long)cpu_state[cpu_bh]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb8:
        cpu_state[cpu_ax] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov ax, 0x%llx\n", (unsigned long long)cpu_state[cpu_ax]);
        cpu_state[cpu_ip]++;
        break;
    case 0xb9:
        cpu_state[cpu_cx] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov cx, 0x%llx\n", (unsigned long long)cpu_state[cpu_cx]);
        cpu_state[cpu_ip]++;
        break;
    case 0xba:
        cpu_state[cpu_dx] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov dx, 0x%llx\n", (unsigned long long)cpu_state[cpu_dx]);
        cpu_state[cpu_ip]++;
        break;
    case 0xbb:
        cpu_state[cpu_bx] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov bx, 0x%llx\n", (unsigned long long)cpu_state[cpu_bx]);
        cpu_state[cpu_ip]++;
        break;
    case 0xbc:
        cpu_state[cpu_sp] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov sp, 0x%llx\n", (unsigned long long)cpu_state[cpu_sp]);
        cpu_state[cpu_ip]++;
        break;
    case 0xbd:
        cpu_state[cpu_bp] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov bp, 0x%llx\n", (unsigned long long)cpu_state[cpu_bp]);
        cpu_state[cpu_ip]++;
        break;
    case 0xbe:
        cpu_state[cpu_si] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov si, 0x%llx\n", (unsigned long long)cpu_state[cpu_si]);
        cpu_state[cpu_ip]++;
        break;
    case 0xbf:
        cpu_state[cpu_di] = cpu_imm16(cpu_ip);
        if (debug)
            printf("mov di, 0x%llx\n", (unsigned long long)cpu_state[cpu_di]);
        cpu_state[cpu_ip]++;
        break;
    case 0xcd:
        value = cpu_imm8(cpu_ip);
        if (value == 0xff)
        {
            exit(0);
        }
        else if (value == 0xfe)
        {
            cpu_state[cpu_ip] = 0;
            break;
        }
        else if (value == 0x20)
        {
            uint32_t pos = window_framebuffer[0] + cpu_state[cpu_dx] * window_framebuffer[3];
            if (pos > vm_memory_size)
            {
                cpu_state[cpu_ip] = 0;
                break;
            }
            vm_memory[pos + 0] = cpu_state[cpu_ax];
            vm_memory[pos + 1] = cpu_state[cpu_bx];
            vm_memory[pos + 2] = cpu_state[cpu_cx];
        }
        if (debug)
            printf("int 0x%llx\n", (unsigned long long)value);
        cpu_state[cpu_ip]++;
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
    unknown:
    default:
        if (debug)
            printf("db 0x%hhx\n", *opcode);
        cpu_state[cpu_ip]++;
        break;
    }
}
