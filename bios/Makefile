CC := clang
CC_FLAGS_BIOS := -c -ffreestanding -I include -m16
BIOSFILES := $(shell find . -name "*.c")
LD_FLAGS_BIOS := --allow-multiple-definition -build-id=none

build:
	@rm -rf $(shell find . -name "*.o") $(shell find . -name "*.bin")
	@$(CC) $(BIOSFILES) $(CC_FLAGS_BIOS)
	@ld -melf_i386 --format binary -Tlinker.ld $(LD_FLAGS_BIOS) $(shell find . -name "*.o") -o bios2.bin