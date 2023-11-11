#pragma once
#pragma clang diagnostic ignored "-Wvarargs"
#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#define abs(a) (((a) < 0) ? -(a) : (a))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define limit(a, b) (((a) > (b)) ? (b) : (a))
#define sign(x) ((x < 0) ? -1 : ((x > 0) ? 1 : 0))s

extern uint64_t vm_memory_size, bios_size;
extern uint8_t *vm_memory;

#endif