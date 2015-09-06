#ifndef _ELF32_H_
#define _ELF32_H_

#define ELFHEAD_MAGICNUM	"\0x7fELF"

/* 32-bit ELF base types. */
typedef unsigned	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned	Elf32_Off;
typedef int	Elf32_Sword;
typedef unsigned	Elf32_Word;

/* 头4字节是 0x7f 'E' 'L' 'F' */
#define EI_NIDENT	16

/* elf32 文件头 */
typedef struct elf32_hdr{
  unsigned char	e_ident[EI_NIDENT];
  Elf32_Half	e_type;         /* ET_XXX */
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;		/* Entry point */
  Elf32_Off		e_phoff;		/* program headers offset */
  Elf32_Off		e_shoff;		/* section headers offset */
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;		/* 本身的长度 */
  Elf32_Half	e_phentsize;	/* Elf32_Phdr描述符结构体长度 */
  Elf32_Half	e_phnum;		/* Elf32_Phdr个数 */
  Elf32_Half	e_shentsize;	/* Elf32_Shdr描述符结构体长度 */
  Elf32_Half	e_shnum;		/* Elf32_Shdr个数 */
  Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

/* These constants define the different elf file types */
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

/* program header 描述符 */
/* 用来加载到内存的 通常可执行的elf会有program header */
typedef struct elf32_phdr{
  Elf32_Word	p_type;         /* PT_XXX */
  Elf32_Off	p_offset;
  Elf32_Addr	p_vaddr;        /*  */
  Elf32_Addr	p_paddr;        /*  */
  Elf32_Word	p_filesz;
  Elf32_Word	p_memsz;
  Elf32_Word	p_flags;
  Elf32_Word	p_align;
} Elf32_Phdr;

/* ph_type */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7               /* Thread local storage segment */
#define PT_LOOS    0x60000000      /* OS-specific */
#define PT_HIOS    0x6fffffff      /* OS-specific */
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff
#define PT_GNU_EH_FRAME		0x6474e550

#define PT_GNU_STACK	(PT_LOOS + 0x474e551)

/* section header 描述符 */
typedef struct elf32_shdr {
  Elf32_Word	sh_name;
  Elf32_Word	sh_type;            /* SHT_XXX */
  Elf32_Word	sh_flags;           /* SHF_XXX */
  Elf32_Addr	sh_addr;            /* 如果section要映射到主存中 则该值反映本section在主存映射的位置 */
  Elf32_Off		sh_offset;			/* 本section在文件中的偏移 */
  Elf32_Word	sh_size;
  Elf32_Word	sh_link;			/* 根据不同的section而不同, 例如当section是符号表的时候，那么link就是符号表各个符号名所在的strtbl */
  Elf32_Word	sh_info;
  Elf32_Word	sh_addralign;
  Elf32_Word	sh_entsize;         /* 像SHT_SYMTAB这样的section 则该域代表每个entry的大小 如果section不是固定大小的entry表 那该域为0 */
} Elf32_Shdr;

/* sh_type */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4               /* 相对于SHT_REL而言 是有显示的addends的 重定向表项的机构多出一项 */
#define SHT_HASH	5
#define SHT_DYNAMIC	6               /* 动态链接的信息 */
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9               /* sh_link:做重定向用到的符号表 sh_info:重定向操作针对的section */
#define SHT_SHLIB	10              /* This section type is reserved but has unspecified semantics. */
#define SHT_DYNSYM	11              /* 每项和SHT_SYMTAB一样 用于动态链接 */
#define SHT_NUM		12
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/* sh_flags */
#define SHF_WRITE	0x1				/* 用于指示section在执行过程中的地址空间必须具有写权限 */
#define SHF_ALLOC	0x2				/* 用于指示section在执行过程中需要载入内存，即需要占据一些执行空间 */
#define SHF_EXECINSTR	0x4			/* 用于指示section中包含可执行代码 */
#define SHF_MASKPROC	0xf0000000	/* 编译器使用特殊位 */

/* special section indexes 正常的直接就是objfile中section表项的索引 但存在以下若干特例 不指向特定的section */
#define SHN_UNDEF	0				/* object文件中 Elf32_Shdr表的第0项是无效项 所以索引0就是一个无效索引 */
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

/* SHT_SYMTAB段中的entry 每个entry表示一个symbol */
typedef struct elf32_sym{
  Elf32_Word	st_name;
  Elf32_Addr	st_value;           /* 对于可重定向文件(-c出来的)而言
                                            如果st_shndx为SHN_COMMON 相当于说这个符号可以任意放 则st_value存的是对齐限制
                                            如果st_shndx为一个有效section索引 则st_value存的是该符号相对与st_shndx指定段起始的偏移位置 
                                       对于可执行文件或者共享库文件而言 该值直接是一个虚拟地址 */
  Elf32_Word	st_size;
  unsigned char	st_info;            /* 高4bit是bind 低4bit是type */
  unsigned char	st_other;           /* This member currently holds 0 and has no defined meaning. */
  Elf32_Half	st_shndx;           /* 每个符号有一个相关的section 例如SHN_UNDEF就说明该symbol没有定义 */
} Elf32_Sym;

/* 以下宏用于Elf32_Sym.st_info */
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

/* Elf32_Sym.st_info高4bit的bind的取值 无论是变量还是函数 都具有bind属性 */
#define STB_LOCAL  0                /* objfile之外不可见的 每个obj内部自己使用 不同objfile可以重名 */
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_LOPORC  13
#define STB_HIPORC  15

/* Elf32_Sym.st_info低4bit的type的取值 */
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPORC  13
#define STT_HIPORC  15


typedef struct elf32_rel {
  Elf32_Addr	r_offset;       /* 对于可重定向文件而言: 该变量表示待重定向的符号在目标section中偏移
                                                        通过sh_info可以找到目标section 然后在base上加上这个r_offset
                                                        则是最终需要做重定向那个符号的位置了
                                   对于可执行文件或共享库文件而言: 该表项表示待重定向的符号所在位置的虚拟地址 */
  Elf32_Word	r_info;         /* 高24bit是该重定向操作的符号在符号表中的索引 符号表在section表中的索引由本section的sh_link字段给出 低8bit是type*/
} Elf32_Rel;

typedef struct elf32_rela{
  Elf32_Addr	r_offset;       /* 同Elf32_Rel.r_offset */
  Elf32_Word	r_info;         /* 同Elf32_Rel.r_info */
  Elf32_Sword	r_addend;
} Elf32_Rela;

/* 用于操作Elf32_Rel.r_info */
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

/* Elf32_Rel.r_info中低bit的重定向类型 */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC	10
#define R_386_NUM	11

#endif
