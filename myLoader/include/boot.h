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
	
	unsigned short	tmp1, tmp2, tmp3, tmp4;
	unsigned char	rsv2[14];
	struct hd_dptentry hd_pdt[4];			/* 64bytes */
	unsigned short bootsectflag;            /* 0xAA55 */
}__attribute__((packed));

#endif

