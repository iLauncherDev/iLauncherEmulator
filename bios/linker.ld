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

	.rodata ALIGN(0x1000) :
	{
		rodata_start = .;
		*(.rodata)
		rodata_end = .;
	}

	.data ALIGN(0x1000) : AT(0xc0000)
	{
		data_start = .;
		*(.data)
		data_end = .;
	}

	.bss ALIGN(0x1000) : AT(data_end)
	{
		bss_start = .;
		*(COMMON)
		*(.bss)
		bss_end = .;
	}

    bios_end = ALIGN(0x1000);

	.null :
	{
		*(.null)
		. += 0xfffff - bios_end;
	}
}