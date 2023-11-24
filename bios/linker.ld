OUTPUT_FORMAT("binary");
ENTRY(main);

SECTIONS {
    . = 0xe0000;
	bios_start = .;

	.text ALIGN(0x1000) :
	{
		text_start = .;
		*(.text)
		text_end = .;
	}

	.data ALIGN(0x1000) : AT(0xc0000)
	{
		data_start = .;
		*(.data)
		data_end = .;
	}

	.rodata ALIGN(0x1000) : AT(data_end)
	{
		rodata_start = .;
		*(.rodata)
		rodata_end = .;
	}

	.bss ALIGN(0x1000) : AT(rodata_end)
	{
		bss_start = .;
		*(COMMON)
		*(.bss)
		bss_end = .;
	}

    bios_end = .;

	.null :
	{
		. += 0xffff0 - bios_end;
		*(.null)
	}
}