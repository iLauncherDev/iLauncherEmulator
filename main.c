#include "emulator/global.h"
#include "emulator/vga.h"
#include "emulator/cpu.h"
#include "emulator/cpu_x86.h"
#include "emulator/io.h"

SDL_Window *window;
SDL_Surface *window_surface;
SDL_Event event;
uint8_t scancode[4096] = {false};
uint8_t scancode_ready[4096] = {false};
uint8_t debug_code = false, dump_bios = false;
uintptr_t ticks, code_delay = 0;
cpu_t *x86_cpu;

uint64_t window_framebuffer[] = {
    0x100000,
    640,
    480,
    4,
    0,
};

uint64_t vm_memory_size, bios_size;
uint8_t *vm_memory, *bios_rom;

void framebuffer_draw(void *input, void *output,
                      uint16_t input_width, uint16_t input_height,
                      uint16_t output_width, uint16_t output_height,
                      uint8_t bpp)
{
    if (!input || !output || !bpp)
        return;
    double scale_x = (double)output_width / (double)input_width;
    double scale_y = (double)output_height / (double)input_height;
    switch (bpp)
    {
    case 4:
        for (uint16_t y = 0; y < output_height; y++)
        {
            uint16_t round_y = (uint16_t)((double)y / scale_y);
            for (uint16_t x = 0; x < output_width; x++)
            {
                uint16_t round_x = (uint16_t)((double)x / scale_x);
                ((uint32_t *)output)[y * output_width + x] =
                    ((uint32_t *)input)[round_y * input_width + round_x];
            }
        }
        break;
    }
}

void *window_update()
{
    uint8_t fullscreen = false;
    memset(scancode, false, 4096);
    memset(scancode_ready, true, 4096);
    while (true)
    {
        window_surface = SDL_GetWindowSurface(window);
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                exit(0);
            }
            else if (event.type == SDL_KEYUP)
            {
                scancode[event.key.keysym.scancode] = false;
                scancode_ready[event.key.keysym.scancode] = true;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                scancode[event.key.keysym.scancode] = true;
            }
        }
        if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
            scancode[SDL_SCANCODE_F] && scancode_ready[SDL_SCANCODE_F])
        {
            if (!fullscreen)
            {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP), fullscreen = true;
            }
            else
            {
                SDL_SetWindowFullscreen(window, 0), fullscreen = false;
                SDL_SetWindowSize(window, window_framebuffer[1], window_framebuffer[2]);
            }
            scancode_ready[SDL_SCANCODE_F] = false;
        }
        else if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
                 scancode[SDL_SCANCODE_Q] && scancode_ready[SDL_SCANCODE_Q])
        {
            exit(0);
            scancode_ready[SDL_SCANCODE_Q] = false;
        }
        else if (scancode[SDL_SCANCODE_LCTRL] && scancode[SDL_SCANCODE_LALT] &&
                 scancode[SDL_SCANCODE_R] && scancode_ready[SDL_SCANCODE_R])
        {
            x86_cpu->flags |= cpu_flag_reset;
            scancode_ready[SDL_SCANCODE_R] = false;
        }
        framebuffer_draw(&vm_memory[window_framebuffer[0]], window_surface->pixels,
                         window_framebuffer[1], window_framebuffer[2],
                         window_surface->w, window_surface->h,
                         4);
        SDL_UpdateWindowSurface(window);
    }
    return (void *)NULL;
}

void *cpu_emulation()
{
    while (true)
    {
        cpu_recompile(x86_cpu);
        //cpu_execute(x86_cpu);
        if (debug_code)
        {
            printf("Regs:\n");
            for (uint16_t i = 0; i < x86_reg_end; i += 8)
                printf("%s = 0x%" PRIx64 " ", x86_regs_strings[i], cpu_read_reg(x86_cpu, i, 8));
            printf("\n");
        }
        if (code_delay)
            usleep(code_delay);
    }
}

int32_t main(int32_t argc, char **argv)
{
    FILE *bios_bin = (FILE *)NULL;
    bios_size = 0;
    for (int32_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-memory"))
        {
            if (argc - i < 2)
                continue;
            i++;
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
            bios_bin = fopen(argv[i], "rb");
            if (!bios_bin)
                continue;
            fseek(bios_bin, 0L, SEEK_END);
            bios_size = ftell(bios_bin);
            fseek(bios_bin, 0L, SEEK_SET);
        }
        else if (!strcmp(argv[i], "-debug-code"))
        {
            debug_code = true;
        }
        else if (!strcmp(argv[i], "-display"))
        {
            window_framebuffer[1] = atoi(argv[i + 1]);
            window_framebuffer[2] = atoi(argv[i + 2]);
            i += 2;
        }
        else
        {
            printf("Unknown Argument: %s\n", argv[i]);
            return 1;
        }
    }
    if (vm_memory_size < 0x800000)
        vm_memory_size = 0x800000;
    vm_memory = malloc(vm_memory_size);
    printf("Allocated %" PRIu64 "MB In RAM\n", vm_memory_size / 1024 / 1024);
    if (bios_bin)
        bios_rom = malloc(bios_size), fread(bios_rom, bios_size, 1, bios_bin);
    memory_map_buffer(MEMORY_READ_FLAG,
                      bios_rom,
                      0xfffff - limit(bios_size, (256 * 1024) - 1),
                      0,
                      bios_size);
    if (dump_bios)
    {
        printf("Address: 0x%" PRIx64 "\n", 0xfffff - limit(bios_size, (256 * 1024) - 1));
        for (size_t i = 0; i < bios_size; i++)
            printf("%" PRIx64 " ", memory_read(i + (0xfffff - limit(bios_size, (256 * 1024) - 1)), 1, 0));
        printf("\n");
        return 0;
    }
    x86_cpu = x86_setup();
    vga_install();
    io_write(0xfff0, window_framebuffer[0], 4);
    io_write(0xfff4, window_framebuffer[1], 2);
    io_write(0xfff6, window_framebuffer[2], 2);
    io_write(0xfff8, window_framebuffer[3], 1);
    window = SDL_CreateWindow("iLEmu", 0, 0,
                              window_framebuffer[1], window_framebuffer[2],
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    pthread_t window_update_thread, cpu_emulation_thread;
    pthread_create(&window_update_thread, NULL, window_update, NULL);
    pthread_create(&cpu_emulation_thread, NULL, cpu_emulation, NULL);
    while (true)
    {
        if (io_read(0x3f8, 1))
            printf("%c", (uint8_t)io_read(0x3f8, 1)), io_write(0x3f8, 0, 1);
        vga_service();
    }
    return 0;
}
