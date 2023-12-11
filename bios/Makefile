CC := clang
CC_FLAGS := -c -ffreestanding -I include -m32 -masm=intel -fno-pie
BIOSFILES := $(shell find . -name "*.c")
LD_FLAGS := -nostdlib -static

build:
	@rm -rf `find . -name "*.o"`
	@nasm -felf32 bios.asm -o bios.o
	@$(CC) $(BIOSFILES) $(CC_FLAGS)
	@ld -Tlinker.ld $(LD_FLAGS) `find . -name "*.o"` -o bios.bin
	@rm -rf `find . -name "*.o"`
	@rm -rf bios.txt && ndisasm -b32 bios.bin >> bios.txt
