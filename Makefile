CC := clang
CC_FLAGS := `sdl2-config --cflags --libs` -Werror -Ofast

build:
	@nasm -fbin test.asm -o bios.bin
	@$(CC) $(shell ls **/*.c; ls *.c) -o iLEmu-system-x86_64 $(CC_FLAGS)

run:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16

run-debug:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code

run-debug-single:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 1000000
