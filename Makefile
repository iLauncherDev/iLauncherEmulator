CC := clang
CC_FLAGS := `sdl2-config --cflags --libs` -Werror

build:
	@nasm -fbin test.asm -o bios.bin
	@$(CC) $(shell ls **/*.c; ls *.c) -o iLEmu-system-x86_64 $(CC_FLAGS)

run:
	@./iLEmu-system-x86_64 -memory 128 -debug
