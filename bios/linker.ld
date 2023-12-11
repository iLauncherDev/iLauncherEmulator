OUTPUT_FORMAT("binary");
OUTPUT_ARCH(i386:i386);
ENTRY(main);

SECTIONS {
    . = 0xe0000;
	bios_start = .;

	.text ALIGN(0x1000) :
	{
		*(.text)
	}
	
	.rodata ALIGN(0x1000) :
	{
		*(.rodata)
	}
	
	.data ALIGN(0x1000) :
	{
		*(.data)
	}
	
	.bss ALIGN(0x1000) :
	{
		*(COMMON)
		*(.bss)
	}

    bios_end = .;

	.null :
	{
		. += 0xffff0 - bios_end;
		*(.null)
	}
	
	bios_end = .;
}
