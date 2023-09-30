OUTPUT_FORMAT("binary");
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

    bios_end = ALIGN(0x1000);

	.null :
	{
		*(.null)
		. += 0xfffff - bios_end;
	}
}