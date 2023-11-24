#pragma once
#ifndef CPU_H
#define CPU_H
#define CPU_CHECK_OVERFLOW(x, y, bits) (((uintptr_t)x + (uintptr_t)y) >> (uintptr_t)bits ? 1 : 0)
#include "global.h"
#include "memory.h"
#include "io.h"

typedef enum cpu_defines
{
    cpu_flag_lock = 1 << 0,
    cpu_flag_reset = 1 << 1,

    cpu_type_reg,
    cpu_type_int,
    cpu_type_memory_reg,
    cpu_type_memory,
} cpu_defines_t;

typedef struct cpu
{
    uint16_t flags, override;
    uint64_t pc, pc_base;
    uint64_t (*read_reg)(struct cpu *cpu, uint16_t reg);
    void (*write_reg)(struct cpu *cpu, uint16_t reg, uint64_t value);
    void (*reset)(struct cpu *cpu);
    uint8_t (*emulate)(struct cpu *cpu);
    uint8_t *regs, *cache;
} cpu_t;

uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg);
void cpu_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value);
void cpu_reset(cpu_t *cpu);
void cpu_emulate(cpu_t *cpu);
#endif