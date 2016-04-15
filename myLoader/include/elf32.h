#ifndef _ELF32_H_
#define _ELF32_H_

#define ELFHEAD_MAGICNUM	"\0x7fELF"

/* 32-bit ELF base types. */
typedef unsigned	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned	Elf32_Off;
typedef int	Elf32_Sword;
typedef unsigned	Elf32_Word;

/* ͷ4�ֽ��� 0x7f 'E' 'L' 'F' */
#define EI_NIDENT	16

/* elf32 �ļ�ͷ */
typedef struct elf32_hdr{
  unsigned char	e_ident[EI_NIDENT];
  Elf32_Half	e_type;         /* ET_XXX */
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;		/* Entry point */
  Elf32_Off		e_phoff;		/* program headers offset */
  Elf32_Off		e_shoff;		/* section headers offset */
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;		/* ����ĳ��� */
  Elf32_Half	e_phentsize;	/* Elf32_Phdr�������ṹ�峤�� */
  Elf32_Half	e_phnum;		/* Elf32_Phdr���� */
  Elf32_Half	e_shentsize;	/* Elf32_Shdr�������ṹ�峤�� */
  Elf32_Half	e_shnum;		/* Elf32_Shdr���� */
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

/* program header ������ */
/* �������ص��ڴ�� ͨ����ִ�е�elf����program header */
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

/* section header ������ */
typedef struct elf32_shdr {
  Elf32_Word	sh_name;
  Elf32_Word	sh_type;            /* SHT_XXX */
  Elf32_Word	sh_flags;           /* SHF_XXX */
  Elf32_Addr	sh_addr;            /* ���sectionҪӳ�䵽������ ���ֵ��ӳ��section������ӳ���λ�� */
  Elf32_Off		sh_offset;			/* ��section���ļ��е�ƫ�� */
  Elf32_Word	sh_size;
  Elf32_Word	sh_link;			/* ���ݲ�ͬ��section����ͬ, ���統section�Ƿ��ű��ʱ����ôlink���Ƿ��ű�������������ڵ�strtbl */
  Elf32_Word	sh_info;
  Elf32_Word	sh_addralign;
  Elf32_Word	sh_entsize;         /* ��SHT_SYMTAB������section ��������ÿ��entry�Ĵ�С ���section���ǹ̶���С��entry�� �Ǹ���Ϊ0 */
} Elf32_Shdr;

/* sh_type */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4               /* �����SHT_REL���� ������ʾ��addends�� �ض������Ļ������һ�� */
#define SHT_HASH	5
#define SHT_DYNAMIC	6               /* ��̬���ӵ���Ϣ */
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9               /* sh_link:���ض����õ��ķ��ű� sh_info:�ض��������Ե�section */
#define SHT_SHLIB	10              /* This section type is reserved but has unspecified semantics. */
#define SHT_DYNSYM	11              /* ÿ���SHT_SYMTABһ�� ���ڶ�̬���� */
#define SHT_NUM		12
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/* sh_flags */
#define SHF_WRITE	0x1				/* ����ָʾsection��ִ�й����еĵ�ַ�ռ�������дȨ�� */
#define SHF_ALLOC	0x2				/* ����ָʾsection��ִ�й�������Ҫ�����ڴ棬����Ҫռ��һЩִ�пռ� */
#define SHF_EXECINSTR	0x4			/* ����ָʾsection�а�����ִ�д��� */
#define SHF_MASKPROC	0xf0000000	/* ������ʹ������λ */

/* special section indexes ������ֱ�Ӿ���objfile��section��������� ������������������ ��ָ���ض���section */
#define SHN_UNDEF	0				/* object�ļ��� Elf32_Shdr��ĵ�0������Ч�� ��������0����һ����Ч���� */
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

/* SHT_SYMTAB���е�entry ÿ��entry��ʾһ��symbol */
typedef struct elf32_sym{
  Elf32_Word	st_name;
  Elf32_Addr	st_value;           /* ���ڿ��ض����ļ�(-c������)����
                                            ���st_shndxΪSHN_COMMON �൱��˵������ſ�������� ��st_value����Ƕ�������
                                            ���st_shndxΪһ����Чsection���� ��st_value����Ǹ÷��������st_shndxָ������ʼ��ƫ��λ�� 
                                       ���ڿ�ִ���ļ����߹�����ļ����� ��ֱֵ����һ�������ַ */
  Elf32_Word	st_size;
  unsigned char	st_info;            /* ��4bit��bind ��4bit��type */
  unsigned char	st_other;           /* This member currently holds 0 and has no defined meaning. */
  Elf32_Half	st_shndx;           /* ÿ��������һ����ص�section ����SHN_UNDEF��˵����symbolû�ж��� */
} Elf32_Sym;

/* ���º�����Elf32_Sym.st_info */
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

/* Elf32_Sym.st_info��4bit��bind��ȡֵ �����Ǳ������Ǻ��� ������bind���� */
#define STB_LOCAL  0                /* objfile֮�ⲻ�ɼ��� ÿ��obj�ڲ��Լ�ʹ�� ��ͬobjfile�������� */
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_LOPORC  13
#define STB_HIPORC  15

/* Elf32_Sym.st_info��4bit��type��ȡֵ */
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPORC  13
#define STT_HIPORC  15


typedef struct elf32_rel {
  Elf32_Addr	r_offset;       /* ���ڿ��ض����ļ�����: �ñ�����ʾ���ض���ķ�����Ŀ��section��ƫ��
                                                        ͨ��sh_info�����ҵ�Ŀ��section Ȼ����base�ϼ������r_offset
                                                        ����������Ҫ���ض����Ǹ����ŵ�λ����
                                   ���ڿ�ִ���ļ�������ļ�����: �ñ����ʾ���ض���ķ�������λ�õ������ַ */
  Elf32_Word	r_info;         /* ��24bit�Ǹ��ض�������ķ����ڷ��ű��е����� ���ű���section���е������ɱ�section��sh_link�ֶθ��� ��8bit��type*/
} Elf32_Rel;

typedef struct elf32_rela{
  Elf32_Addr	r_offset;       /* ͬElf32_Rel.r_offset */
  Elf32_Word	r_info;         /* ͬElf32_Rel.r_info */
  Elf32_Sword	r_addend;
} Elf32_Rela;

/* ���ڲ���Elf32_Rel.r_info */
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

/* Elf32_Rel.r_info�е�bit���ض������� */
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
