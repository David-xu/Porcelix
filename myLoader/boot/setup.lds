SECTIONS
{
	.text :
	{
		*(.text.entry)
		*(.text)
	}
	.data :
	{
		*(.data)
	}
	. = 0x1A2;
	boot_dev = .;			/* boot.S中使用该符号 保存启动时的磁盘号 */
}
