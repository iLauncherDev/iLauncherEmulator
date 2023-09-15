#pragma once
#ifndef CPU_H
#define CPU_H
#include "memory.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>

typedef enum cpu_regs
{
    cpu_reg_gs,
    cpu_reg_fs,
    cpu_reg_es,
    cpu_reg_ds,
    cpu_reg_cs,
    cpu_reg_ss,

    cpu_reg_al,
    cpu_reg_ah,
    cpu_reg_bl,
    cpu_reg_bh,
    cpu_reg_cl,
    cpu_reg_ch,
    cpu_reg_dl,
    cpu_reg_dh,
    cpu_reg_bpl,
    cpu_reg_sil,
    cpu_reg_dil,
    cpu_reg_spl,
    cpu_reg_ipl,
    cpu_reg_flagsl,
    cpu_reg_r8b,
    cpu_reg_r9b,
    cpu_reg_r10b,
    cpu_reg_r11b,
    cpu_reg_r12b,
    cpu_reg_r13b,
    cpu_reg_r14b,
    cpu_reg_r15b,

    cpu_reg_ax,
    cpu_reg_bx,
    cpu_reg_cx,
    cpu_reg_dx,
    cpu_reg_bp,
    cpu_reg_si,
    cpu_reg_di,
    cpu_reg_sp,
    cpu_reg_ip,
    cpu_reg_flags,
    cpu_reg_r8w,
    cpu_reg_r9w,
    cpu_reg_r10w,
    cpu_reg_r11w,
    cpu_reg_r12w,
    cpu_reg_r13w,
    cpu_reg_r14w,
    cpu_reg_r15w,

    cpu_reg_eax,
    cpu_reg_ebx,
    cpu_reg_ecx,
    cpu_reg_edx,
    cpu_reg_ebp,
    cpu_reg_esi,
    cpu_reg_edi,
    cpu_reg_esp,
    cpu_reg_eip,
    cpu_reg_eflags,
    cpu_reg_r8d,
    cpu_reg_r9d,
    cpu_reg_r10d,
    cpu_reg_r11d,
    cpu_reg_r12d,
    cpu_reg_r13d,
    cpu_reg_r14d,
    cpu_reg_r15d,

    cpu_reg_end,

    cpu_cc_a,
    cpu_cc_b,
    cpu_cc_c,
    cpu_cc_d,
    cpu_cc_e,
    cpu_cc_f,

    cpu_cc_end,

    cpu_type_reg,
    cpu_type_int,
    cpu_type_memory_reg,
    cpu_type_memory,
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
    (int8_t *)"sil",
    (int8_t *)"dil",
    (int8_t *)"spl",
    (int8_t *)"ipl",
    (int8_t *)"flagsl",
    (int8_t *)"r8b",
    (int8_t *)"r9b",
    (int8_t *)"r10b",
    (int8_t *)"r11b",
    (int8_t *)"r12b",
    (int8_t *)"r13b",
    (int8_t *)"r14b",
    (int8_t *)"r15b",

    (int8_t *)"ax",
    (int8_t *)"bx",
    (int8_t *)"cx",
    (int8_t *)"dx",
    (int8_t *)"bp",
    (int8_t *)"si",
    (int8_t *)"di",
    (int8_t *)"sp",
    (int8_t *)"ip",
    (int8_t *)"flags",
    (int8_t *)"r8w",
    (int8_t *)"r9w",
    (int8_t *)"r10w",
    (int8_t *)"r11w",
    (int8_t *)"r12w",
    (int8_t *)"r13w",
    (int8_t *)"r14w",
    (int8_t *)"r15w",

    (int8_t *)"eax",
    (int8_t *)"ebx",
    (int8_t *)"ecx",
    (int8_t *)"edx",
    (int8_t *)"ebp",
    (int8_t *)"esi",
    (int8_t *)"edi",
    (int8_t *)"esp",
    (int8_t *)"eip",
    (int8_t *)"eflags",
    (int8_t *)"r8d",
    (int8_t *)"r9d",
    (int8_t *)"r10d",
    (int8_t *)"r11d",
    (int8_t *)"r12d",
    (int8_t *)"r13d",
    (int8_t *)"r14d",
    (int8_t *)"r15d",
};

typedef struct cpu_info
{
    uint8_t segmentation;
    uint8_t reg_type;
    uint8_t reg_type_buffer[3];
} cpu_info_t;

extern uint64_t vm_memory_size;
extern uint8_t *vm_memory;
extern uint64_t cpu_state[];

void cpu_reset();
void cpu_setup_precalcs();
void cpu_dump_state();
void cpu_emulate_i8086(uint8_t debug);

#endif