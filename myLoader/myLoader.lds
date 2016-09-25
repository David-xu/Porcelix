ENTRY(core_entry)			/* boot.bin use this address by readelf */

PHDRS
{
	initseg PT_LOAD AT(ADDR(.initsect)) FLAGS(7);
	coreseg PT_LOAD AT(ADDR(.coresect)) FLAGS(7);
}

SECTIONS
{
	. = 0x1000;

	.initsect : AT(ADDR(.initsect))
	{
		*(.init.start)		/* causetion, only those startup codes in the corestart.S may in this section. */

		*(.init.data)
		*(.coreentry.param)	
		
		gdt_table = .;		/* make the symbol gdt_table */
		*(.gdt.data)
		idt_table = .;		/* make the symbol idt_table */
		*(.idt.data)
	} : initseg
	
	symbol_inittext_end = .;/* we want to know the size of section '.initsect' */
	
	. = 0x4000;

	.text : AT(ADDR(.text))
	{
		*(.init.text)
		*(.text)

	} : coreseg						/* if we don't specify the PHDRS, ld will used the PHDRS before */
	
	.rodata : AT(ADDR(.rodata))
	{
		. = ALIGN(0x40);
		fsdesc_array = .;	/* all fs desc */
		*(.array.fs)		/* take care: ld align the struct(which larger than 32 byte) as 32 byte, so we need to take some pad */
		fsdesc_array_end = .;

		cmddesc_array = .;
		*(.array.cmd)
		cmddesc_array_end = .;

		moduleinit_array_0 = .;
		*(.array.moduleinit_0)
		moduleinit_array_0_end = .;

		moduleinit_array_1 = .;
		*(.array.moduleinit_1)
		moduleinit_array_1_end = .;

		moduleinit_array_2 = .;
		*(.array.moduleinit_2)
		moduleinit_array_2_end = .;

		moduleinit_array_3 = .;
		*(.array.moduleinit_3)
		moduleinit_array_3_end = .;

		moduleinit_array_4 = .;
		*(.array.moduleinit_4)
		moduleinit_array_4_end = .;

		moduleinit_array_5 = .;
		*(.array.moduleinit_5)
		moduleinit_array_5_end = .;

		moduleinit_array_6 = .;
		*(.array.moduleinit_6)
		moduleinit_array_6_end = .;

		moduleinit_array_7 = .;
		*(.array.moduleinit_7)
		moduleinit_array_7_end = .;
		
		syminfo_begin = .;
		*(SORT(.symtbl_info_*))
		syminfo_end = .;
		
		*(.rodata*)
	}
	
	.data : AT(ADDR(.data))
	{
		*(.data)

		*(.bss)

	} : coreseg

	symbol_sys_end = .;	/* this is the sys end position */
	
	/* Sections to be discarded */
	/DISCARD/ : { *(.exit.text) *(.cpuexit.text) *(.memexit.text) *(.exit.data) *(.cpuexit.data) *(.cpuexit.rodata) *(.memexit.data) *(.memexit.rodata) *(.exitcall.exit) *(.discard) *(.discard.*) }
	/DISCARD/ : { *(.eh_frame) }
}
