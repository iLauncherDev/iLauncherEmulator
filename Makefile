GCC_FLAGS := `sdl2-config --cflags --libs` -Werror

build:
	@nasm -fbin test.asm -o bios.bin
	@gcc $(shell ls **/*.c; ls *.c) -o iLEmu-system-x86_64 $(GCC_FLAGS)

run:
	@./iLEmu-system-x86_64 -memory 128