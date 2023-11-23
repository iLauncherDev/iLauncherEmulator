CC := clang
CC_FLAGS_BIOS := -c -ffreestanding -I include -m16 -masm=intel -fno-pie
BIOSFILES := $(shell find . -name "*.c")
LD_FLAGS_BIOS := --allow-multiple-definition -build-id=none

build:
	@rm -rf `find . -name "*.o"`
	@nasm -felf32 bios.asm -o bios.o
	@$(CC) $(BIOSFILES) $(CC_FLAGS_BIOS)
	@ld -melf_i386 -Tlinker.ld $(LD_FLAGS_BIOS) `find . -name "*.o"` -o bios.bin
	@rm -rf `find . -name "*.o"`
	@rm -rf bios.txt && ndisasm -b16 bios.bin >> bios.txt