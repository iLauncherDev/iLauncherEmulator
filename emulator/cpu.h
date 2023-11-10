#pragma once
#ifndef CPU_H
#define CPU_H
#define CPU_CHECK_OVERFLOW(x, y, bits) (((uintptr_t)x + (uintptr_t)y) >> (uintptr_t)bits ? 1 : 0)
#include "global.h"
#include "memory.h"
#include "io.h"

typedef enum cpu_defines
{
    cpu_flag_debug = 1 << 0,
    cpu_flag_crash = 1 << 1,

    cpu_type_reg,
    cpu_type_int,
    cpu_type_memory_reg,
    cpu_type_memory,
} cpu_defines_t;

typedef struct cpu_info
{
    uint16_t flags;
    uint8_t size;
    uint8_t reg_type;
    uint8_t reg_type_buffer[8];
} cpu_info_t;

typedef struct cpu
{
    uint16_t flags, override;
    global_uint64_t pc;
    global_uint64_t (*read_reg)(struct cpu *cpu, uint16_t reg);
    void (*write_reg)(struct cpu *cpu, uint16_t reg, global_uint64_t value);
    void (*reset)(struct cpu *cpu);
    uint8_t (*emulate)(struct cpu *cpu);
    uint8_t *regs, *cache;
    cpu_info_t cpu_info[8];
    uint8_t info_index;
} cpu_t;

global_uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg);
void cpu_write_reg(cpu_t *cpu, uint16_t reg, global_uint64_t value);
void cpu_reset(cpu_t *cpu);
void cpu_emulate(cpu_t *cpu);
#endif