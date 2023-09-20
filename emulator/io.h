#pragma once
#ifndef IO_H
#define IO_H
#include "memory.h"

extern uint8_t io_ports[];

uint64_t io_read(uint16_t port, uint8_t size);
void io_write(uint16_t port, uint64_t value, uint8_t size);
#endif