#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "emulator/cpu.h"

SDL_Window *window;
SDL_Renderer *window_renderer;
SDL_Surface *window_surface;
SDL_Event event;
uint8_t scancode[4096] = {0};

uint64_t window_framebuffer[] = {
    0xb8000,
    80 * 8,
    25 * 16,
    4,
    0,
};

uint64_t vm_memory_size;
uint8_t *vm_memory;

void *window_update()
{
    SDL_Texture *texture;
    uint8_t ready[4096];
    uint8_t fullscreen = false;
    while (true)
    {
        SDL_RenderClear(window_renderer);
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                exit(0);
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
        if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
            scancode[SDL_SCANCODE_LSHIFT] && scancode[SDL_SCANCODE_F] && ready[SDL_SCANCODE_F])
        {
            if (!fullscreen)
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP), fullscreen = true;
            else
                SDL_SetWindowFullscreen(window, 0), fullscreen = false;
            ready[SDL_SCANCODE_F] = false;
            continue;
        }
        else if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
                 scancode[SDL_SCANCODE_LSHIFT] && scancode[SDL_SCANCODE_Q] && ready[SDL_SCANCODE_Q])
        {
            exit(0);
            ready[SDL_SCANCODE_Q] = false;
            continue;
        }
        else
        {
            for (uint16_t i = 0; i < 0x1000; i++)
                if (!scancode[i])
                    ready[i] = true;
        }
        memcpy(window_surface->pixels, &vm_memory[window_framebuffer[0]], window_surface->pitch * window_surface->h);
        texture = SDL_CreateTextureFromSurface(window_renderer, window_surface);
        SDL_RenderCopy(window_renderer, texture, NULL, NULL);
        SDL_DestroyTexture(texture);
        SDL_RenderPresent(window_renderer);
    }
    return (void *)NULL;
}

int32_t main(int32_t argc, char **argv)
{
    uint8_t debug_code = false, dump_bios = false;
    uint32_t code_delay = 0;
    FILE *bios_bin = (FILE *)NULL;
    uint64_t size = 0;
    for (int32_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-memory"))
        {
            if (argc - i < 2)
                continue;
            i++;
            if (vm_memory)
                continue;
            vm_memory = malloc(atoi(argv[i]) * 1024 * 1024);
            if (!vm_memory)
                return 1;
            printf("Allocated %sMB In RAM\n", argv[i]);
            vm_memory_size = atoi(argv[i]) * 1024 * 1024;
        }
        else if (!strcmp(argv[i], "-dump-bios"))
        {
            dump_bios = true;
        }
        else if (!strcmp(argv[i], "-code-delay"))
        {
            if (argc - i < 2)
                continue;
            i++;
            code_delay = atoi(argv[i]);
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
    if (!vm_memory)
        vm_memory = malloc(16 * 1024 * 1024),
        vm_memory_size = 16 * 1024 * 1024,
        printf("Allocated %sMB In RAM\n", "16");
    if (bios_bin)
        fread(vm_memory, size, 1, bios_bin);
    if (dump_bios)
    {
        for (size_t i = 0; i < size; i++)
            printf("%x ", vm_memory[i]);
        printf("\n");
        return 0;
    }
    cpu_setup_precalcs();
    cpu_state[cpu_reg_eip] = cpu_state[cpu_reg_ip] = 0;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Emulator", 0, 0, 80 * 8, 25 * 16, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    window_renderer = SDL_CreateRenderer(window, -1, 0);
    window_surface = SDL_CreateRGBSurface(0, 80 * 8, 25 * 16, 32, 0xff0000, 0x00ff00, 0x0000ff, 0x000000);
    pthread_t window_update_thread;
    pthread_create(&window_update_thread, NULL, window_update, NULL);
    while (!window_framebuffer[0])
        sleep(1);
    while (true)
    {
        cpu_emulate_i8086(debug_code);
        if (code_delay)
            usleep(code_delay);
    }
    return 0;
}
