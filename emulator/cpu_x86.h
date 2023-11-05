#pragma once
#ifndef CPU_X86_H
#define CPU_X86_H
#include "cpu.h"

typedef enum x86_regs
{
    x86_reg_gs = 0,
    x86_reg_fs = x86_reg_gs + 2,
    x86_reg_es = x86_reg_fs + 2,
    x86_reg_ds = x86_reg_es + 2,
    x86_reg_cs = x86_reg_ds + 2,
    x86_reg_ss = x86_reg_cs + 2,

    x86_reg_al = x86_reg_ss + 2,
    x86_reg_ah = x86_reg_al + 1,
    x86_reg_cl = x86_reg_ah + 1,
    x86_reg_ch = x86_reg_cl + 1,
    x86_reg_dl = x86_reg_ch + 1,
    x86_reg_dh = x86_reg_dl + 1,
    x86_reg_bl = x86_reg_dh + 1,
    x86_reg_bh = x86_reg_bl + 1,
    x86_reg_spl = x86_reg_bh + 1,
    x86_reg_bpl = x86_reg_spl + 2,
    x86_reg_sil = x86_reg_bpl + 2,
    x86_reg_dil = x86_reg_sil + 2,
    x86_reg_ipl = x86_reg_dil + 2,
    x86_reg_flagsl = x86_reg_ipl + 2,
    x86_reg_r8b = x86_reg_flagsl + 2,
    x86_reg_r9b = x86_reg_r8b + 2,
    x86_reg_r10b = x86_reg_r9b + 2,
    x86_reg_r11b = x86_reg_r10b + 2,
    x86_reg_r12b = x86_reg_r11b + 2,
    x86_reg_r13b = x86_reg_r12b + 2,
    x86_reg_r14b = x86_reg_r13b + 2,
    x86_reg_r15b = x86_reg_r14b + 2,

    x86_reg_ax = x86_reg_r15b + 2,
    x86_reg_cx = x86_reg_ax + 2,
    x86_reg_dx = x86_reg_cx + 2,
    x86_reg_bx = x86_reg_dx + 2,
    x86_reg_sp = x86_reg_bx + 2,
    x86_reg_bp = x86_reg_sp + 2,
    x86_reg_si = x86_reg_bp + 2,
    x86_reg_di = x86_reg_si + 2,
    x86_reg_ip = x86_reg_di + 2,
    x86_reg_flags = x86_reg_ip + 2,
    x86_reg_r8w = x86_reg_flags + 2,
    x86_reg_r9w = x86_reg_r8w + 2,
    x86_reg_r10w = x86_reg_r9w + 2,
    x86_reg_r11w = x86_reg_r10w + 2,
    x86_reg_r12w = x86_reg_r11w + 2,
    x86_reg_r13w = x86_reg_r12w + 2,
    x86_reg_r14w = x86_reg_r13w + 2,
    x86_reg_r15w = x86_reg_r14w + 2,

    x86_reg_eax = x86_reg_r15w + 2,
    x86_reg_ecx = x86_reg_eax + 4,
    x86_reg_edx = x86_reg_ecx + 4,
    x86_reg_ebx = x86_reg_edx + 4,
    x86_reg_esp = x86_reg_ebx + 4,
    x86_reg_ebp = x86_reg_esp + 4,
    x86_reg_esi = x86_reg_ebp + 4,
    x86_reg_edi = x86_reg_esi + 4,
    x86_reg_eip = x86_reg_edi + 4,
    x86_reg_eflags = x86_reg_eip + 4,
    x86_reg_r8d = x86_reg_eflags + 4,
    x86_reg_r9d = x86_reg_r8d + 4,
    x86_reg_r10d = x86_reg_r9d + 4,
    x86_reg_r11d = x86_reg_r10d + 4,
    x86_reg_r12d = x86_reg_r11d + 4,
    x86_reg_r13d = x86_reg_r12d + 4,
    x86_reg_r14d = x86_reg_r13d + 4,
    x86_reg_r15d = x86_reg_r14d + 4,

    x86_reg_gdtr = x86_reg_r15d + 4,

    x86_reg_end = x86_reg_gdtr + 8,

    x86_flags_CF = 1 << 0,
    x86_flags_PF = 1 << 2,
    x86_flags_AF = 1 << 4,
    x86_flags_ZF = 1 << 6,
    x86_flags_SF = 1 << 7,
    x86_flags_TF = 1 << 8,
    x86_flags_IF = 1 << 9,
    x86_flags_DF = 1 << 10,
    x86_flags_OF = 1 << 11,

    x86_cache_rm = 0,
    x86_cache_mod = x86_cache_rm + 1,
    x86_cache_reg = x86_cache_mod + 1,
    x86_cache_address = x86_cache_reg + 1,
    x86_cache_address_old = x86_cache_address + 8,
    x86_cache_opcode = x86_cache_address_old + 8,
    x86_cache_override_dword_operand = x86_cache_opcode + 1,
    x86_cache_override_dword_address = x86_cache_override_dword_operand + 1,
    x86_cache_override_gs = x86_cache_override_dword_address + 1,
    x86_cache_override_fs = x86_cache_override_gs + 1,
    x86_cache_override_es = x86_cache_override_fs + 1,
    x86_cache_override_ds = x86_cache_override_es + 1,
    x86_cache_override_cs = x86_cache_override_ds + 1,
    x86_cache_override_ss = x86_cache_override_cs + 1,
} x86_regs_t;

static const char *x86_regs_strings[x86_reg_end] = {
    "gs",
    (char *)NULL,
    "fs",
    (char *)NULL,
    "es",
    (char *)NULL,
    "ds",
    (char *)NULL,
    "cs",
    (char *)NULL,
    "ss",
    (char *)NULL,

    "al",
    "ah",
    "cl",
    "ch",
    "dl",
    "dh",
    "bl",
    "bh",
    "spl",
    (char *)NULL,
    "bpl",
    (char *)NULL,
    "sil",
    (char *)NULL,
    "dil",
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
    "cx",
    (char *)NULL,
    "dx",
    (char *)NULL,
    "bx",
    (char *)NULL,
    "sp",
    (char *)NULL,
    "bp",
    (char *)NULL,
    "si",
    (char *)NULL,
    "di",
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
    "ecx",
    (char *)NULL,
    (char *)NULL,
    "edx",
    (char *)NULL,
    (char *)NULL,
    "ebx",
    (char *)NULL,
    (char *)NULL,
    "esp",
    (char *)NULL,
    (char *)NULL,
    "ebp",
    (char *)NULL,
    (char *)NULL,
    "esi",
    (char *)NULL,
    (char *)NULL,
    "edi",
    (char *)NULL,
    (char *)NULL,
    "eip",
    (char *)NULL,
    (char *)NULL,
    "eflags",
    (char *)NULL,
    (char *)NULL,
    "r8d",
    (char *)NULL,
    (char *)NULL,
    "r9d",
    (char *)NULL,
    (char *)NULL,
    "r10d",
    (char *)NULL,
    (char *)NULL,
    "r11d",
    (char *)NULL,
    (char *)NULL,
    "r12d",
    (char *)NULL,
    (char *)NULL,
    "r13d",
    (char *)NULL,
    (char *)NULL,
    "r14d",
    (char *)NULL,
    (char *)NULL,
    "r15d",
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

static const char *x86_rm16_strings[8] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx",
};

struct cpu *x86_setup();
#endif