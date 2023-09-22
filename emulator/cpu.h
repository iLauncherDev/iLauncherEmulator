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

static int8_t *cpu_regs_string[] = {
    (int8_t *)"gs",
    (int8_t *)"fs",
    (int8_t *)"es",
    (int8_t *)"ds",
    (int8_t *)"cs",
    (int8_t *)"ss",

    (int8_t *)"al",
    (int8_t *)"ah",
    (int8_t *)"bl",
    (int8_t *)"bh",
    (int8_t *)"cl",
    (int8_t *)"ch",
    (int8_t *)"dl",
    (int8_t *)"dh",
    (int8_t *)"bpl",
    (int8_t *)NULL,
    (int8_t *)"sil",
    (int8_t *)NULL,
    (int8_t *)"dil",
    (int8_t *)NULL,
    (int8_t *)"spl",
    (int8_t *)NULL,
    (int8_t *)"ipl",
    (int8_t *)NULL,
    (int8_t *)"flagsl",
    (int8_t *)NULL,
    (int8_t *)"r8b",
    (int8_t *)NULL,
    (int8_t *)"r9b",
    (int8_t *)NULL,
    (int8_t *)"r10b",
    (int8_t *)NULL,
    (int8_t *)"r11b",
    (int8_t *)NULL,
    (int8_t *)"r12b",
    (int8_t *)NULL,
    (int8_t *)"r13b",
    (int8_t *)NULL,
    (int8_t *)"r14b",
    (int8_t *)NULL,
    (int8_t *)"r15b",
    (int8_t *)NULL,

    (int8_t *)"ax",
    (int8_t *)NULL,
    (int8_t *)"bx",
    (int8_t *)NULL,
    (int8_t *)"cx",
    (int8_t *)NULL,
    (int8_t *)"dx",
    (int8_t *)NULL,
    (int8_t *)"bp",
    (int8_t *)NULL,
    (int8_t *)"si",
    (int8_t *)NULL,
    (int8_t *)"di",
    (int8_t *)NULL,
    (int8_t *)"sp",
    (int8_t *)NULL,
    (int8_t *)"ip",
    (int8_t *)NULL,
    (int8_t *)"flags",
    (int8_t *)NULL,
    (int8_t *)"r8w",
    (int8_t *)NULL,
    (int8_t *)"r9w",
    (int8_t *)NULL,
    (int8_t *)"r10w",
    (int8_t *)NULL,
    (int8_t *)"r11w",
    (int8_t *)NULL,
    (int8_t *)"r12w",
    (int8_t *)NULL,
    (int8_t *)"r13w",
    (int8_t *)NULL,
    (int8_t *)"r14w",
    (int8_t *)NULL,
    (int8_t *)"r15w",
    (int8_t *)NULL,

    (int8_t *)"eax",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"ebx",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"ecx",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"edx",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"ebp",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"esi",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"edi",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"esp",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"eip",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"eflags",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r8d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r9d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r10d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r11d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r12d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r13d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r14d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)"r15d",
    (int8_t *)NULL,
    (int8_t *)NULL,
    (int8_t *)NULL,
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