#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <SDL2/SDL.h>
#include "emulator/cpu.h"

SDL_Window *window;
SDL_Surface *window_surface;
SDL_Event event;
uint8_t scancode[4096];

uint64_t window_framebuffer[] = {
    0,
    0,
    0,
    0,
    0,
};

uint8_t *vram;

void *window_update()
{
    uint8_t ready = true;
    uint8_t fullscreen = false;
    while (true)
    {
        window_surface = SDL_GetWindowSurface(window);
        window_framebuffer[0] = 0xb8000;
        window_framebuffer[1] = window_surface->w;
        window_framebuffer[2] = window_surface->h;
        window_framebuffer[3] = window_surface->pitch / window_surface->w;
        if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
            scancode[SDL_SCANCODE_LSHIFT] && scancode[SDL_SCANCODE_F] && ready)
        {
            if (!fullscreen)
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN), fullscreen = true;
            else
                SDL_SetWindowFullscreen(window, 0), fullscreen = false;
            ready = false;
        }
        else if (!scancode[SDL_SCANCODE_LCTRL] && !scancode[SDL_SCANCODE_LALT] &&
                 !scancode[SDL_SCANCODE_LSHIFT] && !scancode[SDL_SCANCODE_F])
        {
            ready = true;
        }
        memcpy(window_surface->pixels, &vram[window_framebuffer[0]], window_surface->pitch * window_surface->h);
        SDL_UpdateWindowSurface(window);
    }
    return (void *)NULL;
}

int32_t main(int32_t argc, char **argv)
{
    uint8_t debug_code = false;
    for (int32_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-memory"))
        {
            if (vram)
                continue;
            vram = malloc(atoi(argv[i + 1]) * 1024 * 1024);
            if (!vram)
                return 1;
            printf("Allocated %sMB In RAM\n", argv[i + 1]);
            i++;
        }
        else if (!strcmp(argv[i], "-debug-code"))
        {
            debug_code = true;
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
    window = SDL_CreateWindow("Emulator", 0, 0, 80 * 8, 25 * 16, SDL_WINDOW_SHOWN);
    window_surface = SDL_GetWindowSurface(window);
    pthread_t window_update_thread;
    pthread_create(&window_update_thread, NULL, window_update, NULL);
    extern void sleep(uint32_t);
    uint8_t a = 0;
    while (true)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                return 0;
            }
            else if (event.type == SDL_KEYUP)
            {
                scancode[event.key.keysym.scancode] = true;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                scancode[event.key.keysym.scancode] = false;
            }
        }
        cpu_emulate_i8086(debug_code);
    }
    return 0;
}
