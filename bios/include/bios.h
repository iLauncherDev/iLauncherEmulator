#pragma once
#ifndef BIOS_H
#define BIOS_H
#define true 1
#define false 0
#define null false
#define TRUE 1
#define FALSE 0
#define NULL FALSE

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

static inline void memcpy(void *dest, void *src, uint32_t size)
{
    while (size--)
        *(uint8_t *)dest++ = *(uint8_t *)src++;
}

static inline void memset(void *dest, uint8_t value, uint32_t size)
{
    while (size--)
        *(uint8_t *)dest++ = value;
}
#endif