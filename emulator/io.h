#pragma once
#ifndef IO_H
#define IO_H
#define IO_READ_FLAG (1 << 0)
#define IO_WRITE_FLAG (1 << 1)
#include "global.h"

extern uint8_t io_ports[];

uint8_t io_check_flag(uint16_t port, uint8_t flag, uint8_t size);
void io_clear_flag(uint16_t port, uint8_t flag, uint8_t size);
uint64_t io_read(uint16_t port, uint8_t size);
void io_write(uint16_t port, uint64_t value, uint8_t size);
#endif