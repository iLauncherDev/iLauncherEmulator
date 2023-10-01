#pragma once
#ifndef BIOS_H
#define BIOS_H
#include <io.h>
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

void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);
#endif