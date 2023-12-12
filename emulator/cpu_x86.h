#pragma once
#ifndef CPU_X86_H
#define CPU_X86_H
#include "cpu.h"

typedef enum x86_regs
{
    x86_reg_gs = 0,
    x86_reg_fs = x86_reg_gs + 8,
    x86_reg_es = x86_reg_fs + 8,
    x86_reg_ds = x86_reg_es + 8,
    x86_reg_cs = x86_reg_ds + 8,
    x86_reg_ss = x86_reg_cs + 8,

    x86_reg_eax = x86_reg_ss + 8,
    x86_reg_ecx = x86_reg_eax + 8,
    x86_reg_edx = x86_reg_ecx + 8,
    x86_reg_ebx = x86_reg_edx + 8,
    x86_reg_esp = x86_reg_ebx + 8,
    x86_reg_ebp = x86_reg_esp + 8,
    x86_reg_esi = x86_reg_ebp + 8,
    x86_reg_edi = x86_reg_esi + 8,
    x86_reg_eip = x86_reg_edi + 8,
    x86_reg_eflags = x86_reg_eip + 8,

    x86_reg_cr0 = x86_reg_eflags + 8,
    x86_reg_cr1 = x86_reg_cr0 + 8,
    x86_reg_cr2 = x86_reg_cr1 + 8,
    x86_reg_cr3 = x86_reg_cr2 + 8,
    x86_reg_cr4 = x86_reg_cr3 + 8,
    x86_reg_cr5 = x86_reg_cr4 + 8,
    x86_reg_cr6 = x86_reg_cr5 + 8,
    x86_reg_cr7 = x86_reg_cr6 + 8,
    x86_reg_cr8 = x86_reg_cr7 + 8,
    x86_reg_cr9 = x86_reg_cr8 + 8,
    x86_reg_cr10 = x86_reg_cr9 + 8,
    x86_reg_cr11 = x86_reg_cr10 + 8,
    x86_reg_cr12 = x86_reg_cr11 + 8,
    x86_reg_cr13 = x86_reg_cr12 + 8,
    x86_reg_cr14 = x86_reg_cr13 + 8,
    x86_reg_cr15 = x86_reg_cr14 + 8,

    x86_reg_gdtr = x86_reg_cr15 + 8,
    x86_reg_ldtr = x86_reg_gdtr + 8,
    x86_reg_idtr = x86_reg_ldtr + 8,

    x86_reg_end = x86_reg_idtr + 8,

    x86_flags_CF = 1 << 0,
    x86_flags_PF = 1 << 2,
    x86_flags_AF = 1 << 4,
    x86_flags_ZF = 1 << 6,
    x86_flags_SF = 1 << 7,
    x86_flags_TF = 1 << 8,
    x86_flags_IF = 1 << 9,
    x86_flags_DF = 1 << 10,
    x86_flags_OF = 1 << 11,
    x86_flags_IOPL = 1 << 12,
    x86_flags_NT = 1 << 14,
    x86_flags_RF = 1 << 16,
    x86_flags_VM = 1 << 17,
    x86_flags_AC = 1 << 18,
    x86_flags_VIF = 1 << 19,
    x86_flags_VIP = 1 << 20,
    x86_flags_ID = 1 << 21,

    x86_cr0_PE = 1 << 0,
    x86_cr0_MP = 1 << 1,
    x86_cr0_EM = 1 << 2,
    x86_cr0_TS = 1 << 3,
    x86_cr0_ET = 1 << 4,
    x86_cr0_NE = 1 << 5,
    x86_cr0_WP = 1 << 16,
    x86_cr0_AM = 1 << 18,
    x86_cr0_NW = 1 << 29,
    x86_cr0_CD = 1 << 30,
    x86_cr0_PG = 1 << 31,

    x86_cache_size = 0,
    x86_cache_address0 = x86_cache_size + 1,
    x86_cache_address1 = x86_cache_address0 + 8,
    x86_cache_seg_gs = x86_cache_address1 + 8,
    x86_cache_seg_fs = x86_cache_seg_gs + 8,
    x86_cache_seg_es = x86_cache_seg_fs + 8,
    x86_cache_seg_ds = x86_cache_seg_es + 8,
    x86_cache_seg_cs = x86_cache_seg_ds + 8,
    x86_cache_seg_ss = x86_cache_seg_cs + 8,
    x86_cache_end = x86_cache_seg_ss + 8,

    x86_operation_add = 0x00,
    x86_operation_or = 0x01,
    x86_operation_adc = 0x02,
    x86_operation_sbb = 0x03,
    x86_operation_and = 0x04,
    x86_operation_sub = 0x05,
    x86_operation_xor = 0x06,
    x86_operation_cmp = 0x07,
    x86_operation_xchg = 0x08,
    x86_operation_mov = 0x09,

    x86_override_dword_operand = 1 << 0,
    x86_override_dword_address = 1 << 1,
    x86_override_qword_operand_address = 1 << 2,
    x86_override_gs = 1 << 3,
    x86_override_fs = 1 << 4,
    x86_override_es = 1 << 5,
    x86_override_ds = 1 << 6,
    x86_override_cs = 1 << 7,
    x86_override_ss = 1 << 8,
} x86_regs_t;

static const uint16_t x86_regscontrol[] = {
    x86_reg_cr0,
    x86_reg_cr1,
    x86_reg_cr2,
    x86_reg_cr3,
    x86_reg_cr4,
    x86_reg_cr5,
    x86_reg_cr6,
    x86_reg_cr7,
};

static const uint16_t x86_mod3_reg[][8] = {
    {},
    {
        x86_reg_eax,
        x86_reg_ecx,
        x86_reg_edx,
        x86_reg_ebx,
        x86_reg_eax + 1,
        x86_reg_ecx + 1,
        x86_reg_edx + 1,
        x86_reg_ebx + 1,
    },
    {
        x86_reg_eax,
        x86_reg_ecx,
        x86_reg_edx,
        x86_reg_ebx,
        x86_reg_esp,
        x86_reg_ebp,
        x86_reg_esi,
        x86_reg_edi,
    },
    {},
    {
        x86_reg_eax,
        x86_reg_ecx,
        x86_reg_edx,
        x86_reg_ebx,
        x86_reg_esp,
        x86_reg_ebp,
        x86_reg_esi,
        x86_reg_edi,
    },
};

static const uint16_t x86_regs[] = {
    x86_reg_eax,
    x86_reg_ecx,
    x86_reg_edx,
    x86_reg_ebx,
    x86_reg_esp,
    x86_reg_ebp,
    x86_reg_esi,
    x86_reg_edi,
};

static const uint16_t x86_regs8[] = {
    x86_reg_eax,
    x86_reg_ecx,
    x86_reg_edx,
    x86_reg_ebx,
    x86_reg_eax + 1,
    x86_reg_ecx + 1,
    x86_reg_edx + 1,
    x86_reg_ebx + 1,
};

static const uint16_t x86_sregs[] = {
    x86_reg_es,
    x86_reg_cs,
    x86_reg_ss,
    x86_reg_ds,
    x86_reg_fs,
    x86_reg_gs,
};

typedef union x86_rm
{
    uint8_t value;
    struct
    {
        uint8_t rm : 3;
        uint8_t reg : 3;
        uint8_t mod : 2;
    };
} x86_rm_t;

typedef union x86_sib
{
    uint8_t value;
    struct
    {
        uint8_t base : 3;
        uint8_t index : 3;
        uint8_t scale : 2;
    };
} x86_sib_t;

static const char *x86_regs_strings[x86_reg_end] = {
    "gs",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "fs",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "es",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ds",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cs",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ss",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,

    "eax",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ecx",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "edx",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ebx",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "esp",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "ebp",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "esi",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "edi",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "eip",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "eflags",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,

    "cr0",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr1",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr2",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr3",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr4",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr5",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr6",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr7",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr8",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr9",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr10",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr11",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr12",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr13",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr14",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    "cr15",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
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

    "ldtr",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,

    "idtr",
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
    (char *)NULL,
};

struct cpu *x86_setup();
#endif