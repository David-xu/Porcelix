SECTIONS
{
	.text :
	{
		*(.init.start)			/* causetion, only those startup codes in the corestart.S may in this section. */
		*(.init.text)
		symbol_inittext_end = .;/* we want to know the size of section '.init' */

		*(.text)
	}
	.bss :
	{
		
	}
	.data :
	{
		gdt_table = .;		/* make the symbol gdt_table */
		*(.gdt.data)
		idt_table = .;		/* make the symbol idt_table */
		*(.idt.data)

		fsdesc_array = .;	/* all fs desc */
		*(.array.fs)		/* take care: ld align the struct(which larger than 32 byte) as 32 byte, so we need to take some pad */
		fsdesc_array_end = .;

		cmddesc_array = .;
		*(.array.cmd)
		cmddesc_array_end = .;

		*(.data)

		*(.init.data)

		symbol_sys_end = .;	/* this is the sys end position */
	}
}
