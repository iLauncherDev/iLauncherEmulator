#pragma once
#ifndef CPU_H
#define CPU_H
#define CPU_CHECK_OVERFLOW(x, y, bits) (((uintptr_t)x + (uintptr_t)y) >> (uintptr_t)bits ? 1 : 0)
#include "global.h"
#include "memory.h"
#include "io.h"

typedef enum cpu_defines
{
    cpu_neutral_reg_instruction_pointer = 0,
    cpu_neutral_reg_flags,
    cpu_neutral_flag_carry,
    cpu_neutral_flag_parity,
    cpu_neutral_flag_auxiliary,
    cpu_neutral_flag_zero,
    cpu_neutral_flag_sign,
    cpu_neutral_flag_overfollow,
    cpu_neutral_flag_interrupt,

    cpu_opcode_quit = 0,
    cpu_opcode_mov,
    cpu_opcode_xchg,
    cpu_opcode_jmp_near,
    cpu_opcode_jcc_near,

    cpu_flag_lock = 1 << 0,
    cpu_flag_reset = 1 << 1,
    cpu_flag_jump = 1 << 2,

    // Ex: 0x1000;
    cpu_type_int = 0,
    // Ex: ax;
    cpu_type_reg,
    // Ex: [0x1000];
    cpu_type_mem,
} cpu_defines_t;

typedef struct cpu_packet
{
    uint8_t instruction, opcode_size;
    uint8_t sign : 1, value_length : 2;
    struct cpu_packet_value
    {
        uint8_t type : 2, size;
        uint64_t value;
    } values[1 << 2];
} cpu_packet_t;

typedef struct cpu
{
    uint8_t big_endian : 1;
    uint32_t flags, override;
    uint64_t pc, pc_base;
    uint64_t neutral_values[16];
    void (*push)(struct cpu *cpu, uint64_t value);
    uint64_t (*pop)(struct cpu *cpu);
    uint64_t (*read_reg)(struct cpu *cpu, uint16_t reg, uint8_t size);
    int64_t (*sread_reg)(struct cpu *cpu, uint16_t reg, uint8_t size);
    void (*write_reg)(struct cpu *cpu, uint16_t reg, uint64_t value, uint8_t size);
    void (*reset)(struct cpu *cpu);
    uint8_t (*recompile)(struct cpu *cpu);
    cpu_packet_t code_packet[4096];
    uint16_t code_packet_index;
    uint8_t *regs, *cache;
} cpu_t;

uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg, uint8_t size);
int64_t cpu_sread_reg(cpu_t *cpu, uint16_t reg, uint8_t size);
void cpu_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value, uint8_t size);
void cpu_reset(cpu_t *cpu);
void cpu_recompile(cpu_t *cpu);
void cpu_execute(cpu_t *cpu);
#endif