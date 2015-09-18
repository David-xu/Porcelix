PHDRS
{
	bigseg PT_LOAD AT(0);
}

SECTIONS
{
	. = 0x80000000;
	.bigsect :
	{
		*(.text)
		*(.rodata*)
		*(.data)
		
	} : bigseg
}
