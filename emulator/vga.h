#pragma once
#ifndef VGA_H
#define VGA_H
#include "io.h"
#include "memory.h"
#include "global.h"
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_MISC_READ 0x3CC
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA
#define VGA_NUM_SEQ_REGS 5
#define VGA_NUM_CRTC_REGS 25
#define VGA_NUM_GC_REGS 9
#define VGA_NUM_AC_REGS 21
#define VGA_NUM_REGS (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

typedef struct vga
{
    uint8_t buffer[524288];
    uint8_t misc;
    uint8_t seq[VGA_NUM_SEQ_REGS];
    uint8_t crtc[VGA_NUM_CRTC_REGS];
    uint8_t gc[VGA_NUM_GC_REGS];
    uint8_t ac[VGA_NUM_AC_REGS];
} vga_t;

void vga_service();
void vga_render_frame(uint32_t *rgb_buffer);
#endif