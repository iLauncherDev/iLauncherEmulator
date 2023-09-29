OUTPUT_FORMAT("binary");
ENTRY(main);

SECTIONS {
    . = 0xe0000;

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

    kernel_end = ALIGN(0x1000);
}