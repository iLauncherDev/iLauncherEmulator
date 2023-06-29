#ifndef CPU_H
#define CPU_H
#include "../lib/ctype.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

typedef enum cpu_regs
{
    cpu_gs,
    cpu_fs,
    cpu_es,
    cpu_ds,
    cpu_cs,
    cpu_ss,

    cpu_ah,
    cpu_al,
    cpu_bh,
    cpu_bl,
    cpu_ch,
    cpu_cl,
    cpu_dh,
    cpu_dl,

    cpu_di,
    cpu_si,
    cpu_bp,
    cpu_sp,
    cpu_bx,
    cpu_dx,
    cpu_cx,
    cpu_ax,
    cpu_ip,

    cpu_edi,
    cpu_esi,
    cpu_ebp,
    cpu_esp,
    cpu_ebx,
    cpu_edx,
    cpu_ecx,
    cpu_eax,
    cpu_eip,

    cpu_rdi,
    cpu_rsi,
    cpu_rbp,
    cpu_rsp,
    cpu_rbx,
    cpu_rdx,
    cpu_rcx,
    cpu_rax,
    cpu_rip,
    cpu_r8,
    cpu_r9,
    cpu_r10,
    cpu_r11,
    cpu_r12,
    cpu_r13,
    cpu_r14,
    cpu_r15,

    cpu_eflags,
    cpu_useresp,

    cpu_type_reg,
    cpu_type_int,
    cpu_type_buffer_reg,
    cpu_type_buffer,

    cpu_instruction_add,
    cpu_instruction_sub,
    cpu_instruction_mul,
    cpu_instruction_div,
    cpu_instruction_and,
    cpu_instruction_or,
    cpu_instruction_mov,
} cpu_regs_t;

static int8_t *cpu_regs_string[] = {
    "gs",
    "fs",
    "es",
    "ds",
    "cs",
    "ss",

    "ah",
    "al",
    "bh",
    "bl",
    "ch",
    "cl",
    "dh",
    "dl",

    "di",
    "si",
    "bp",
    "sp",
    "bx",
    "dx",
    "cx",
    "ax",
    "ip",

    "edi",
    "esi",
    "ebp",
    "esp",
    "ebx",
    "edx",
    "ecx",
    "eax",
    "eip",

    "rdi",
    "rsi",
    "rbp",
    "rsp",
    "rbx",
    "rdx",
    "rcx",
    "rax",
    "rip",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",

    "eflags",
    "useresp",
};

extern uint8_t *vram;
extern uint64_t cpu_state[51];

void cpu_dump_state();
void cpu_exec_instruction(uint16_t instruction, uint64_t reg1, uint64_t reg2, uint8_t reg1_type, uint8_t reg2_type);
void cpu_emulate_i386();

#endif