#pragma once
#ifndef CPU_H
#define CPU_H
#define CPU_CHECK_OVERFLOW(x, y, bits) (((uintptr_t)x + (uintptr_t)y) >> (uintptr_t)bits ? 1 : 0)
#define CPU_BLOCK_VALUE(type, size, value, offset) type, size, value, offset
#define CPU_BLOCK_MREG_VALUE(size, base, index, scale, offset) cpu_type_mreg, size,                     \
                                                               (((uint64_t)scale & 0xffffffff) << 32) | \
                                                                   (((uint64_t)index & 0xffff) << 16) | \
                                                                   ((uint64_t)base & 0xffff),           \
                                                               offset
#define CPU_BLOCK_SIZE 4096
#include "global.h"
#include "memory.h"
#include "io.h"

typedef enum cpu_defines
{
    cpu_neutral_reg_instruction_pointer = 0,
    cpu_neutral_reg_code_segment,
    cpu_neutral_reg_flags,
    cpu_neutral_reg_stack,
    cpu_neutral_flag_carry,
    cpu_neutral_flag_parity,
    cpu_neutral_flag_auxiliary,
    cpu_neutral_flag_zero,
    cpu_neutral_flag_sign,
    cpu_neutral_flag_overfollow,
    cpu_neutral_flag_interrupt,

    cpu_opcode_exit = 0,
    cpu_opcode_cmp,
    cpu_opcode_inc,
    cpu_opcode_dec,
    cpu_opcode_push,
    cpu_opcode_pop,
    cpu_opcode_mov,
    cpu_opcode_lea,
    cpu_opcode_xchg,
    cpu_opcode_jmp_far,
    cpu_opcode_jcc_far,
    cpu_opcode_call_far,
    cpu_opcode_ret_far,
    cpu_opcode_jmp_near,
    cpu_opcode_jcc_near,
    cpu_opcode_call_near,
    cpu_opcode_ret_near,

    cpu_flag_lock = 1 << 0,
    cpu_flag_reset = 1 << 1,
    cpu_flag_stoped = 1 << 2,

    // Ex: immediate integer;
    cpu_type_int = 0,
    // Ex: immediate reg;
    cpu_type_reg,
    // Ex: [immediate address];
    cpu_type_mem,
    // Ex: [base + index * scale + offset];
    cpu_type_mreg,
} cpu_defines_t;

typedef struct cpu_block
{
    uint64_t pc;
    uint8_t instruction, opcode_size;
    uint8_t sign : 1, value_length : 2;
    struct cpu_block_value
    {
        uint8_t type : 2, size;
        uint64_t value;
        int64_t offset;
    } values[1 << 2];
} cpu_block_t;

typedef struct cpu
{
    uint8_t big_endian : 1, regs_size;
    uint32_t flags, override;
    uint64_t pc;
    uint64_t neutral_values[16];
    void (*push)(struct cpu *cpu, uint64_t value);
    uint64_t (*pop)(struct cpu *cpu);
    uint64_t (*read_reg)(struct cpu *cpu, uint16_t reg, uint8_t size);
    int64_t (*sread_reg)(struct cpu *cpu, uint16_t reg, uint8_t size);
    void (*write_reg)(struct cpu *cpu, uint16_t reg, uint64_t value, uint8_t size);
    void (*reset)(struct cpu *cpu);
    uint8_t (*recompile)(struct cpu *cpu);
    cpu_block_t code_block[CPU_BLOCK_SIZE];
    uint16_t code_block_index;
    uint8_t *regs, *cache;
} cpu_t;

void cpu_block_add(cpu_t *cpu, uint8_t instruction, uint8_t sign, uint8_t value_length, ...);
uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg, uint8_t size);
int64_t cpu_sread_reg(cpu_t *cpu, uint16_t reg, uint8_t size);
void cpu_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value, uint8_t size);
void cpu_reset(cpu_t *cpu);
void cpu_recompile(cpu_t *cpu);
void cpu_execute(cpu_t *cpu);
#endif