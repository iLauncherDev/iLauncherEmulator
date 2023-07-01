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
uint8_t scancode[4096] = {0};

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
        if (window_surface->w != window_framebuffer[1] ||
            window_surface->h != window_framebuffer[2])
        {
            window_framebuffer[0] = 0xb8000;
            window_framebuffer[1] = window_surface->w;
            window_framebuffer[2] = window_surface->h;
            window_framebuffer[3] = window_surface->pitch / window_surface->w;
        }
        if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
            scancode[SDL_SCANCODE_LSHIFT] && scancode[SDL_SCANCODE_F] && ready)
        {
            if (!fullscreen)
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN), fullscreen = true;
            else
                SDL_SetWindowFullscreen(window, 0), fullscreen = false;
            ready = false;
        }
        else if (!scancode[SDL_SCANCODE_F] && !ready)
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
    FILE *bios_bin = (FILE *)NULL;
    uint64_t size = 0;
    for (int32_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-memory"))
        {
            if (argc - i < 2)
                continue;
            i++;
            if (vram)
                continue;
            vram = malloc(atoi(argv[i]) * 1024 * 1024);
            if (!vram)
                return 1;
            printf("Allocated %sMB In RAM\n", argv[i]);
        }
        else if (!strcmp(argv[i], "-bios"))
        {
            if (argc - i < 2)
                continue;
            i++;
            if (bios_bin)
                continue;
            bios_bin = fopen(argv[i], "rwb");
            if (!bios_bin)
                continue;
            fseek(bios_bin, 0L, SEEK_END);
            size = ftell(bios_bin);
            fseek(bios_bin, 0L, SEEK_SET);
        }
        else if (!strcmp(argv[i], "-debug-code"))
        {
            debug_code = true;
        }
        else
        {
            printf("Unknown Argument: %s\n", argv[i]);
            return 1;
        }
    }
    if (!vram)
        vram = malloc(128 * 1024 * 1024), printf("Allocated %sMB In RAM\n", "128");
    if (bios_bin)
        fread((void *)vram, size, 1, bios_bin);
    cpu_state[cpu_eip] = cpu_state[cpu_ip] = 0;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Emulator", 0, 0, 80 * 8, 25 * 16, SDL_WINDOW_SHOWN);
    window_surface = SDL_GetWindowSurface(window);
    pthread_t window_update_thread;
    pthread_create(&window_update_thread, NULL, window_update, NULL);
    while (!window_framebuffer[0])
        ;
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
                scancode[event.key.keysym.scancode] = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                scancode[event.key.keysym.scancode] = true;
            }
        }
        for (size_t t = 0; t < 0xffff; t++)
            cpu_emulate_i8086(debug_code);
    }
    return 0;
}
