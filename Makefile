CC := clang
CC_FLAGS := `sdl2-config --cflags --libs` -Ofast -Wall -Werror
EMUFILES := $(shell find *.c) $(shell find emulator -name "*.c")
BIOSROM := bios/bios.bin

build:
	@make -C bios build
	@$(CC) $(EMUFILES) -o iLEmu-system-x86_64 $(CC_FLAGS)

run:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480

run-debug:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code

run-debug-1S:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 1000000

run-debug-500MS:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 500000

run-debug-250MS:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 250000

run-debug-125MS:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 250000

run-debug-62MS:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 62500

run-debug-31MS:
	@./iLEmu-system-x86_64 -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 31250