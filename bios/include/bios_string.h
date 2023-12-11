#pragma once
#ifndef BIOS_STRING_H
#define BIOS_STRING_H
#include <bios.h>

void memcpy(void *dest, void *src, uint32_t size);
void memset(void *dest, uint8_t value, uint32_t size);
#endif