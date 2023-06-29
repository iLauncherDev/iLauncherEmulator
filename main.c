#include "lib/ctype.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <SDL2/SDL.h>
#include "emulator/cpu.h"

SDL_Window *window;
SDL_Surface *window_surface;
SDL_Event event;

uint8_t *vram;

void *window_update()
{
    while (true)
    {
        window_surface = SDL_GetWindowSurface(window);
        memcpy(window_surface->pixels, &vram[0xb0000], window_surface->pitch * window_surface->h);
        SDL_UpdateWindowSurface(window);
    }
}

int32_t main(int32_t argc, int8_t **argv)
{
    uint8_t debug_cpu_state = false;
    for (int32_t i = 1; i < argc; i++)
    {
        if (!strcmp((const int8_t *)argv[i], "-memory"))
        {
            if (vram)
                continue;
            vram = malloc(atoi(argv[i + 1]) * 1024 * 1024);
            if (!vram)
                return 1;
            printf("Allocated %sMB In RAM\n", argv[i + 1]);
            i++;
        }
        else if (!strcmp((const int8_t *)argv[i], "-debug"))
        {
            debug_cpu_state = true;
        }
        else
        {
            return 1;
        }
    }
    if (!vram)
        vram = malloc(128 * 1024 * 1024);
    FILE *test_ptr = fopen("bios.bin", "rb");
    uint64_t size;
    fseek(test_ptr, 0L, SEEK_END);
    size = ftell(test_ptr);
    fseek(test_ptr, 0L, SEEK_SET);
    fread((void *)vram, size, 1, test_ptr);
    cpu_state[cpu_eip] = cpu_state[cpu_ip] = 0;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Emulator", 0, 0, 80 * 8, 25 * 16, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    window_surface = SDL_GetWindowSurface(window);
    pthread_t window_update_thread;
    pthread_create(&window_update_thread, NULL, window_update, NULL);
    while (true)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                return 0;
            }
        }
        cpu_emulate_i386();
        if (debug_cpu_state)
            cpu_dump_state();
    }
    return 0;
}
