#ifndef CPU_H
#define CPU_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>

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

    cpu_flags,
    cpu_eflags,
    cpu_rflags,
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
    cpu_instruction_push,
    cpu_instruction_pop,
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

extern uint64_t vm_memory_size;
extern uint8_t *vm_memory;
extern uint64_t cpu_state[];

void cpu_setup_precalcs();
void cpu_dump_state();
void cpu_emulate_i8086(uint8_t debug);
void cpu_emulate_i386(uint8_t debug);

#endif