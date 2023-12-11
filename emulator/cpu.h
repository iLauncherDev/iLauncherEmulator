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
    cpu_flag_jump = 1 << 2,

    // Ex: 0x200;
    cpu_type_int = 0,
    // Ex: ax;
    cpu_type_reg,
    // Ex: [0x200];
    cpu_type_mem,
    // Ex: 0x200, 0x400;
    cpu_type_int_int,
    // Ex: ax, cx;
    cpu_type_reg_reg,
    // Ex: ax, 0x200;
    cpu_type_reg_int,
    // Ex: [0x400], cx;
    cpu_type_mem_reg,
    // Ex: [0x400], 0xff;
    cpu_type_mem_int,
    // Ex: ax, [0x200];
    cpu_type_reg_mem,
    // Ex: [0x400], [0x200];
    cpu_type_mem_mem,
} cpu_defines_t;

typedef struct cpu
{
    uint8_t big_endian : 1;
    uint32_t flags, override;
    uint64_t pc, pc_base;
    uint64_t pc_new, pc_base_new;
    uint64_t (*read_reg)(struct cpu *cpu, uint16_t reg);
    int64_t (*sread_reg)(struct cpu *cpu, uint16_t reg);
    void (*write_reg)(struct cpu *cpu, uint16_t reg, uint64_t value);
    void (*reset)(struct cpu *cpu);
    uint8_t (*emulate)(struct cpu *cpu);
    uint8_t (*execute_packet)(struct cpu *cpu);
    struct cpu_packet
    {
        uint8_t operation;
        uint8_t type : 4, sign : 1, completed : 1, size;
        uint64_t reg_x;
        uint64_t reg_y;
    } code_packet[256];
    uint8_t code_packet_index;
    uint8_t exec_code_packet_index;
    uint8_t *regs, *cache;
} cpu_t;

uint64_t cpu_read_reg(cpu_t *cpu, uint16_t reg);
int64_t cpu_sread_reg(cpu_t *cpu, uint16_t reg);
void cpu_write_reg(cpu_t *cpu, uint16_t reg, uint64_t value);
void cpu_reset(cpu_t *cpu);
void cpu_emulate(cpu_t *cpu);
void cpu_add_code_packet(cpu_t *cpu, uint8_t operantion, uint8_t sign, uint8_t type,
                         uint64_t reg_x, uint64_t reg_y, uint8_t size);
void cpu_execute_packet(cpu_t *cpu);
#endif