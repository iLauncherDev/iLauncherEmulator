#ifndef CPU_H
#define CPU_H
#include "_memory.h"
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

    cpu_reg_ah,
    cpu_reg_al,
    cpu_reg_bh,
    cpu_reg_bl,
    cpu_reg_ch,
    cpu_reg_cl,
    cpu_reg_dh,
    cpu_reg_dl,

    cpu_reg_di,
    cpu_reg_si,
    cpu_reg_bp,
    cpu_reg_sp,
    cpu_reg_bx,
    cpu_reg_dx,
    cpu_reg_cx,
    cpu_reg_ax,
    cpu_reg_ip,

    cpu_reg_edi,
    cpu_reg_esi,
    cpu_reg_ebp,
    cpu_reg_esp,
    cpu_reg_ebx,
    cpu_reg_edx,
    cpu_reg_ecx,
    cpu_reg_eax,
    cpu_reg_eip,

    cpu_reg_rdi,
    cpu_reg_rsi,
    cpu_reg_rbp,
    cpu_reg_rsp,
    cpu_reg_rbx,
    cpu_reg_rdx,
    cpu_reg_rcx,
    cpu_reg_rax,
    cpu_reg_rip,
    cpu_reg_r8,
    cpu_reg_r9,
    cpu_reg_r10,
    cpu_reg_r11,
    cpu_reg_r12,
    cpu_reg_r13,
    cpu_reg_r14,
    cpu_reg_r15,

    cpu_reg_flags,
    cpu_reg_eflags,
    cpu_reg_rflags,
    cpu_reg_useresp,

    cpu_type_reg,
    cpu_type_int,
    cpu_type_buffer_reg,
    cpu_type_buffer,
} cpu_regs_t;

static int8_t *cpu_regs_string[] = {
    (int8_t *)"gs",
    (int8_t *)"fs",
    (int8_t *)"es",
    (int8_t *)"ds",
    (int8_t *)"cs",
    (int8_t *)"ss",

    (int8_t *)"ah",
    (int8_t *)"al",
    (int8_t *)"bh",
    (int8_t *)"bl",
    (int8_t *)"ch",
    (int8_t *)"cl",
    (int8_t *)"dh",
    (int8_t *)"dl",

    (int8_t *)"di",
    (int8_t *)"si",
    (int8_t *)"bp",
    (int8_t *)"sp",
    (int8_t *)"bx",
    (int8_t *)"dx",
    (int8_t *)"cx",
    (int8_t *)"ax",
    (int8_t *)"ip",

    (int8_t *)"edi",
    (int8_t *)"esi",
    (int8_t *)"ebp",
    (int8_t *)"esp",
    (int8_t *)"ebx",
    (int8_t *)"edx",
    (int8_t *)"ecx",
    (int8_t *)"eax",
    (int8_t *)"eip",

    (int8_t *)"rdi",
    (int8_t *)"rsi",
    (int8_t *)"rbp",
    (int8_t *)"rsp",
    (int8_t *)"rbx",
    (int8_t *)"rdx",
    (int8_t *)"rcx",
    (int8_t *)"rax",
    (int8_t *)"rip",
    (int8_t *)"r8",
    (int8_t *)"r9",
    (int8_t *)"r10",
    (int8_t *)"r11",
    (int8_t *)"r12",
    (int8_t *)"r13",
    (int8_t *)"r14",
    (int8_t *)"r15",

    (int8_t *)"flags",
    (int8_t *)"eflags",
    (int8_t *)"rflags",
    (int8_t *)"useresp",
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