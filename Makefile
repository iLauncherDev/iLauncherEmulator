CC := clang
CC_FLAGS := `sdl2-config --cflags --libs` -Werror -Ofast

build:
	@nasm -fbin test.asm -o bios.bin
	@$(CC) $(shell ls **/*.c; ls *.c) -o iLEmu-system-x86_64 $(CC_FLAGS)

run:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16

run-debug:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code

run-debug-1S:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 1000000

run-debug-500MS:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 500000

run-debug-250MS:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 250000

run-debug-125MS:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 250000

run-debug-62MS:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 62500

run-debug-31MS:
	@./iLEmu-system-x86_64 -bios bios.bin -memory 16 -debug-code -code-delay 31250