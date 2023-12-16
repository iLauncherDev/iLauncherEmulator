CC := clang
CC_FLAGS := `sdl2-config --cflags --libs` -Wno-varargs -Wall -Werror
EXEC_OUTPUT := iLEmu-system
EMUFILES := $(shell find *.c) $(shell find emulator -name "*.c")
BIOSROM := bios/bios.bin

build:
	@make -C bios build
	@$(CC) $(EMUFILES) -o $(EXEC_OUTPUT) $(CC_FLAGS)

run:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480

run-debug:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code

run-debug-1S:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 1000000

run-debug-500MS:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 500000

run-debug-250MS:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 250000

run-debug-125MS:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 250000

run-debug-62MS:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 62500

run-debug-31MS:
	@./$(EXEC_OUTPUT) -bios $(BIOSROM) -memory 16 -display 640 480 -debug-code -code-delay 31250