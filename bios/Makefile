CC := clang
CC_FLAGS_BIOS := -c -ffreestanding -I include -m16 -masm=intel -fno-pie
BIOSFILES := $(shell find . -name "*.c")
LD_FLAGS_BIOS := --allow-multiple-definition -build-id=none

build:
	@rm -rf $(LD_FILES)
	@nasm -felf32 bios.asm -o bios.o
	@$(CC) $(BIOSFILES) $(CC_FLAGS_BIOS)
	@ld -melf_i386 -Tlinker.ld $(LD_FLAGS_BIOS) $(shell find . -name "*.o") -o bios.bin
	@rm -rf $(LD_FILES)