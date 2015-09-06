#ifndef _BOOT_H_
#define _BOOT_H_

/* the hd partion table store in address: 0x7c00 + 0x01BE */
struct hd_dptentry{
    unsigned char	boot_ind;
    unsigned char	start_head;
    unsigned short	start_sect: 6;
    unsigned short	start_cyl : 10;
    unsigned char	sys_ind;		/* 0xb-DOS, 0x80-OldMinix, 0x83-Linux */
    unsigned char	end_head;
    unsigned short	end_sect: 6;
    unsigned short	end_cyl : 10;
    unsigned		start_logicsect;	/* this is the partition start logicsect
                                       in whole harddisk sector range */
    unsigned		n_logicsect;
}__attribute__((packed));


/* 96bytes */
struct bootparam {
	unsigned short	n_sect;                  /* sector num of core */
	unsigned char	boot_dev;
	unsigned char	rsv1;
	unsigned		core_crc;
	unsigned		head4k_crc;

	unsigned		core_lba;
	unsigned char	idtr_back[6];
	unsigned char	rsv2[8];
	struct hd_dptentry hd_pdt[4];			/* 64bytes */
	unsigned short bootsectflag;            /* 0xAA55 */
}__attribute__((packed));

extern void jump2pe(void);

/* some info of kernel we wan't to boot */

/*
 * linux boot protocol
 *
 01F1/1	ALL(1	setup_sects	The size of the setup in sectors
 01F2/2	ALL	root_flags	If set, the root is mounted readonly
 01F4/4	2.04+(2	syssize		The size of the 32-bit code in 16-byte paras
 01F8/2	ALL	ram_size	DO NOT USE - for bootsect.S use only
 01FA/2	ALL	vid_mode	Video mode control
 01FC/2	ALL	root_dev	Default root device number
 01FE/2	ALL	boot_flag	0xAA55 magic number
 0200/2	2.00+	jump		Jump instruction
 0202/4	2.00+	header		Magic signature "HdrS"
 0206/2	2.00+	version		Boot protocol version supported
 0208/4	2.00+	realmode_swtch	Boot loader hook (see below)
 020C/2	2.00+	start_sys_seg	The load-low segment (0x1000) (obsolete)
 020E/2	2.00+	kernel_version	Pointer to kernel version string
 0210/1	2.00+	type_of_loader	Boot loader identifier
 0211/1	2.00+	loadflags	Boot protocol option flags
 0212/2	2.00+	setup_move_size	Move to high memory size (used with hooks)
 0214/4	2.00+	code32_start	Boot loader hook (see below)
 0218/4	2.00+	ramdisk_image	initrd load address (set by boot loader)
 021C/4	2.00+	ramdisk_size	initrd size (set by boot loader)
 0220/4	2.00+	bootsect_kludge	DO NOT USE - for bootsect.S use only
 0224/2	2.01+	heap_end_ptr	Free memory after setup end
 0226/1	2.02+(3 ext_loader_ver	Extended boot loader version
 0227/1	2.02+(3	ext_loader_type	Extended boot loader ID
 0228/4	2.02+	cmd_line_ptr	32-bit pointer to the kernel command line
 022C/4	2.03+	ramdisk_max	Highest legal initrd address
 0230/4	2.05+	kernel_alignment Physical addr alignment required for kernel
 0234/1	2.05+	relocatable_kernel Whether kernel is relocatable or not
 0235/1	2.10+	min_alignment	Minimum alignment, as a power of two
 0236/2	2.12+	xloadflags	Boot protocol option flags
 0238/4	2.06+	cmdline_size	Maximum size of the kernel command line
 023C/4	2.07+	hardware_subarch Hardware subarchitecture
 0240/8	2.07+	hardware_subarch_data Subarchitecture-specific data
 0248/4	2.08+	payload_offset	Offset of kernel payload
 024C/4	2.08+	payload_length	Length of kernel payload
 0250/8	2.09+	setup_data	64-bit physical pointer to linked list				of struct setup_data
 0258/8	2.10+	pref_address	Preferred loading address
 0260/4	2.10+	init_size	Linear memory required during initialization
 0264/4	2.11+	handover_offset	Offset of handover entry point
 */
typedef struct _linux_boothead {
	unsigned char		pad[0x1f1];
	unsigned char		setup_sects;
	unsigned short		root_flags;
	unsigned			syssize;
	unsigned short		ram_size;
	unsigned short		vid_mode;
	unsigned short		root_dev;
	unsigned short		boot_flag;

	unsigned short		jmpinstrct;		/* 0x0200 0xeb 0x66 */
	unsigned			magic;			/* "HdrS" */
	unsigned short		version;
	unsigned			realmode_swtch;
	unsigned short		start_sys_seg;
	unsigned short		kernel_version;

	unsigned char		type_of_loader;	/* 0x0210 */
	unsigned char		loadflags;
	unsigned short		setup_move_size;
	unsigned			code32_start;
	unsigned			ramdisk_image;
	unsigned			ramdisk_size;
	
	unsigned			bootsect_kludge;/* 0x0220 */
	unsigned short		heap_end_ptr;
	unsigned char		ext_loader_ver;
	unsigned char		ext_loader_type;
	unsigned			cmd_line_ptr;
	unsigned			ramdisk_max;
	
	unsigned			kernel_alignment;/* 0x0230 */
	unsigned char		relocatable_kernel;
	unsigned char		min_alignment;
	unsigned short		xloadflags;
	unsigned			cmdline_size;
	unsigned			hardware_subarch;

	unsigned long long	hardware_subarch_data;	/* 0x0240 */
	unsigned			payload_offset;
	unsigned			payload_length;

	unsigned long long	setup_data;		/* 0x0250 */
	unsigned long long	pref_address;

	unsigned			init_size;		/* 0x0260 */
	unsigned			handover_offset;
} __attribute__((packed)) linux_boothead_t;

#endif

