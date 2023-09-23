#pragma once
#ifndef CPU_H
#define CPU_H
#include "memory.h"
#include "io.h"
#include "gdt.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>

typedef enum cpu_regs
{
    cpu_reg_gs = 0,
    cpu_reg_fs = 1,
    cpu_reg_es = 2,
    cpu_reg_ds = 3,
    cpu_reg_cs = 4,
    cpu_reg_ss = 5,

    cpu_reg_al = 6,
    cpu_reg_ah = 7,
    cpu_reg_bl = 8,
    cpu_reg_bh = 9,
    cpu_reg_cl = 10,
    cpu_reg_ch = 11,
    cpu_reg_dl = 12,
    cpu_reg_dh = 13,
    cpu_reg_bpl = 14,
    cpu_reg_sil = 16,
    cpu_reg_dil = 18,
    cpu_reg_spl = 20,
    cpu_reg_ipl = 22,
    cpu_reg_flagsl = 24,
    cpu_reg_r8b = 26,
    cpu_reg_r9b = 28,
    cpu_reg_r10b = 30,
    cpu_reg_r11b = 32,
    cpu_reg_r12b = 34,
    cpu_reg_r13b = 36,
    cpu_reg_r14b = 38,
    cpu_reg_r15b = 40,

    cpu_reg_ax = 42,
    cpu_reg_bx = 44,
    cpu_reg_cx = 46,
    cpu_reg_dx = 48,
    cpu_reg_bp = 50,
    cpu_reg_si = 52,
    cpu_reg_di = 54,
    cpu_reg_sp = 56,
    cpu_reg_ip = 58,
    cpu_reg_flags = 60,
    cpu_reg_r8w = 62,
    cpu_reg_r9w = 64,
    cpu_reg_r10w = 66,
    cpu_reg_r11w = 68,
    cpu_reg_r12w = 70,
    cpu_reg_r13w = 72,
    cpu_reg_r14w = 74,
    cpu_reg_r15w = 76,

    cpu_reg_eax = 80,
    cpu_reg_ebx = 84,
    cpu_reg_ecx = 88,
    cpu_reg_edx = 92,
    cpu_reg_ebp = 96,
    cpu_reg_esi = 100,
    cpu_reg_edi = 104,
    cpu_reg_esp = 108,
    cpu_reg_eip = 112,
    cpu_reg_eflags = 116,
    cpu_reg_r8d = 120,
    cpu_reg_r9d = 124,
    cpu_reg_r10d = 128,
    cpu_reg_r11d = 132,
    cpu_reg_r12d = 140,
    cpu_reg_r13d = 144,
    cpu_reg_r14d = 148,
    cpu_reg_r15d = 152,

    cpu_reg_gdtr = 156,

    cpu_reg_end = 164,

    cpu_type_reg,
    cpu_type_int,
    cpu_type_memory_reg,
    cpu_type_memory,

    cpu_flags_CF = 1 << 0,
    cpu_flags_PF = 1 << 2,
    cpu_flags_AF = 1 << 4,
    cpu_flags_ZF = 1 << 6,
    cpu_flags_SF = 1 << 7,
    cpu_flags_TF = 1 << 8,
    cpu_flags_IF = 1 << 9,
    cpu_flags_DF = 1 << 10,
    cpu_flags_OF = 1 << 11,
} cpu_regs_t;

static char *cpu_regs_string[] = {
    "gs",
    "fs",
    "es",
    "ds",
    "cs",
    "ss",

    "al",
    "ah",
    "bl",
    "bh",
    "cl",
    "ch",
    "dl",
    "dh",
    "bpl",
    (char *)NULL,
    "sil",
    (char *)NULL,
    "dil",
    (char *)NULL,
    "spl",
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
    "bx",
    (char *)NULL,
    "cx",
    (char *)NULL,
    "dx",
    (char *)NULL,
    "bp",
    (char *)NULL,
    "si",
    (char *)NULL,
    "di",
    (char *)NULL,
    "sp",
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
    (char *)NULL,
    "ebx",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ecx",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "edx",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ebp",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "esi",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "edi",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "esp",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "eip",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "eflags",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r8d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r9d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r10d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r11d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r12d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r13d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r14d",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "r15d",
    (char *)NULL,
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

typedef struct cpu_info
{
    uint8_t segmentation;
    uint8_t reg_type;
    uint8_t reg_type_buffer[3];
} cpu_info_t;

extern uint8_t cpu_state[];

uint64_t cpu_read_reg(uint8_t reg);
void cpu_write_reg(uint8_t reg, uint64_t value);
void cpu_reset();
void cpu_setup_precalcs();
void cpu_dump_state();
void cpu_emulate_i8086(uint8_t debug);

#endif