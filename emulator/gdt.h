#pragma once
#ifndef GDT_H
#define GDT_H
#include "global.h"
#include "memory.h"
#include "cpu.h"

typedef struct gdt_entry_t
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdtr_t
{
    uint16_t limit;
    global_uint64_t base;
} __attribute__((packed)) gdtr_t;

extern gdtr_t *gdtr;

void gdt_setup();
#endif