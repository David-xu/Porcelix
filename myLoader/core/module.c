#include "config.h"
#include "typedef.h"
#include "public.h"
#include "boot.h"
#include "module.h"
#include "io.h"
#include "debug.h"
#include "memory.h"
#include "command.h"
#include "fs.h"
#include "partition.h"
#include "elf32.h"

char **allsym;

static memcache_t *modcache;

moduleinitlist_t moduleinitlist[] =
{
    {GET_SYMBOLVALUE(moduleinit_array_0), GET_SYMBOLVALUE(moduleinit_array_0_end)},
    {GET_SYMBOLVALUE(moduleinit_array_1), GET_SYMBOLVALUE(moduleinit_array_1_end)},
    {GET_SYMBOLVALUE(moduleinit_array_2), GET_SYMBOLVALUE(moduleinit_array_2_end)},
    {GET_SYMBOLVALUE(moduleinit_array_3), GET_SYMBOLVALUE(moduleinit_array_3_end)},
    {GET_SYMBOLVALUE(moduleinit_array_4), GET_SYMBOLVALUE(moduleinit_array_4_end)},
    {GET_SYMBOLVALUE(moduleinit_array_5), GET_SYMBOLVALUE(moduleinit_array_5_end)},
    {GET_SYMBOLVALUE(moduleinit_array_6), GET_SYMBOLVALUE(moduleinit_array_6_end)},
    {GET_SYMBOLVALUE(moduleinit_array_7), GET_SYMBOLVALUE(moduleinit_array_7_end)},
};

static LIST_HEAD(kmodlist);

static void symtbl_prep(void)
{
	u32 i;
	char *pos;
	/* calc the allsym name pos */
	pos = allsym_namebuff;
	allsym = (char **)page_alloc(get_pagerank(sz_allsymnamebuff), MMAREA_NORMAL);
	for (i = 0; i < n_allsym; i++)
	{
		allsym[i] = pos;
		pos += strlen(pos) + 1;
	}
	ASSERT((pos - allsym_namebuff) == sz_allsymnamebuff);

	printf("total %d symbols in the mld.\n", n_allsym);

	/* init modcache, which used to store all module desc */
	modcache = memcache_create(sizeof(kmodule_t), BUDDY_RANK_4K, "kmodule_desc");
	ASSERT(modcache != NULL);

}

/* 最先初始化全局符号表 打印调用栈需要用到 */
module_init(symtbl_prep, 0);

/*
 * call the init func with each module from level 0 to level 7
 */
void init_module(void)
{
    u32 *p, i, initfuncstat[ARRAY_ELEMENT_SIZE(moduleinitlist)];
    moduleinit init;

	for (i = 0; i < ARRAY_ELEMENT_SIZE(moduleinitlist); i++)
    {
        initfuncstat[i] = 0;
        for (p = (u32 *)(moduleinitlist[i].begin);
             p < (u32 *)(moduleinitlist[i].end);
             p++)
        {
			init = (moduleinit)(*p);
			init();
			initfuncstat[i]++;
        }
    }
	
    printf("module init count:\n");
    for (i = 0; i < ARRAY_ELEMENT_SIZE(moduleinitlist); i++)
    {
        printf("l(%d):%#2x ", i, initfuncstat[i]);
    }
	printf("\n");

}

char *allsym_resolvaddr(u32 addr, u32 *baseaddr)
{
	u32 idx = alg_bsch(addr, allsym_addr, n_allsym);
	*baseaddr = allsym_addr[idx];
	return allsym[idx];
}

static int symname_cmp(const void *key, const void *elt)
{
	const ksyminfo *ks = elt;
	return strcmp((const char *)key, ks->name);
}

ksyminfo *find_symb(char *name)
{
	/* now we just find symbol in the original symlist */
	return bsearch(name, syminfo_begin, syminfo_end - syminfo_begin, sizeof(ksyminfo), symname_cmp);
}

/*
 * return the section header index, which with name
 */
static int findsectbyname(kmodule_t *mod, char *name, Elf32_Ehdr *ehdr, Elf32_Shdr *shdr)
{
	int i, namelen = strlen(name);
	void *p, *namebuf = mod->modbuff + shdr[mod->sect_idx.sect_name].sh_offset;

	for (i = 0; i < ehdr->e_shnum; i++)
	{
		p = namebuf + shdr[i].sh_name;
		if (strlen(p) != namelen)
			continue;
		if (memcmp(name, p, namelen) == 0)
			return i;
	}
	
	return -1;
}

static int resolve_symb(kmodule_t *mod, Elf32_Ehdr *ehdr, Elf32_Shdr *shdr)
{
	int i;
	Elf32_Sym *sym;
	ksyminfo *ksyminfo;
	char *symname;
	

	/* find symbol table section */
	for (i = 0; i < ehdr->e_shnum; i++)
		if (shdr[i].sh_type == SHT_SYMTAB)
			break;
	if (i == ehdr->e_shnum)
	{
		printf("can't find symbol table section in module file.\n");
		return -1;
	}
	ASSERT(shdr[i].sh_entsize == sizeof(Elf32_Sym));
	mod->sect_idx.symtbl = i;
	mod->sect_idx.symname = shdr[mod->sect_idx.symtbl].sh_link;

	sym = mod->modbuff + shdr[mod->sect_idx.symtbl].sh_offset;
	
	for (i = 1; i < (shdr[mod->sect_idx.symtbl].sh_size / sizeof(Elf32_Sym)); i++)
	{
		switch (sym[i].st_shndx)
		{
		case SHN_UNDEF:
			/* need to resolve this symbol */
			symname = (char *)(mod->modbuff + shdr[mod->sect_idx.symname].sh_offset + sym[i].st_name);
			ksyminfo = find_symb(symname);
			if (ksyminfo)
			{
				sym[i].st_value = ksyminfo->addr;
				break;
			}

			/* weak symbol is ok. */
			if (ELF32_ST_BIND(sym[i].st_info) == STB_WEAK)
				break;

			printf("can't resolve sym \"%s\" in mld export symlist.\n", symname);
			return -1;

		case SHN_COMMON:
			ERROR("SHN_COMMON symbol should NOT exist when used \"-no-common\" compile parameter.\n");
			break;
		case SHN_ABS:
			DEBUG("ABS sym: %s\n", mod->modbuff + shdr[mod->sect_idx.symname].sh_offset + sym[i].st_name);
			break;
		default:
			/* modify the symbol with section base address */
			sym[i].st_value += (u32)(mod->modbuff + shdr[sym[i].st_shndx].sh_offset);
			break;
		}
	}
	
	return 0;
}

static int relocate_sym_32(kmodule_t *mod, Elf32_Shdr *shdr, int destsect, int relsect, int symtbl)
{
	char *symname;
	u32 i, *location, symidx;
	Elf32_Rel *rel = (Elf32_Rel *)(mod->modbuff + shdr[relsect].sh_offset);
	Elf32_Sym *sym = (Elf32_Sym *)(mod->modbuff + shdr[symtbl].sh_offset);

	ASSERT(shdr[relsect].sh_entsize == sizeof(Elf32_Rel));
	
	for (i = 0; i < (shdr[relsect].sh_size / sizeof(Elf32_Rel)); i++)
	{
		location = (u32 *)(mod->modbuff + shdr[destsect].sh_offset + rel[i].r_offset);
		symidx = ELF32_R_SYM(rel[i].r_info);
		symname = mod->modbuff +
				  shdr[shdr[symtbl].sh_link].sh_offset +			/* symbol name's strtbl section base */
				  sym[symidx].st_name;

		DEBUG("relocate symbol %s(in symtbl value 0x%#8x) in section %d.\n", symname, sym[symidx].st_value, destsect);

		switch (ELF32_R_TYPE(rel[i].r_info))
		{
		case R_386_32:
			*location += sym[symidx].st_value;
			break;
		case R_386_PC32:
			*location += sym[symidx].st_value - (u32)location;
			break;
		default:
			printf("unknown rel type: %d sym: %s\n", ELF32_R_TYPE(rel[i].r_info), symname);
			return -1;
		}
	}

	return 0;
}

static int do_relocate(kmodule_t *mod, Elf32_Ehdr *ehdr, Elf32_Shdr *shdr)
{
	int i, destsect;
	for (i = 1; i < ehdr->e_shnum; i++)
	{
		/* sh_info indicate the section which need to relocate */
		destsect = shdr[i].sh_info;

		/* may it happen? */
		if (destsect >= ehdr->e_shnum)
			continue;

		/* each section with SHF_ALLOC flag need to do relocation */
		if ((shdr[destsect].sh_flags & SHF_ALLOC) == 0)
			continue;

		if (shdr[i].sh_type == SHT_REL)
		{
			relocate_sym_32(mod, shdr, destsect, i, shdr[i].sh_link);
		}
		else
		{
			ASSERT(0);
		}
	}
	return 0;
}


static int modfile_load(kmodule_t *mod)
{
	int idx;
	Elf32_Ehdr	*ehdr = mod->modbuff;
	Elf32_Shdr	*shdr = (Elf32_Shdr *)(mod->modbuff + ehdr->e_shoff);

	if (memcmp(ehdr->e_ident, ELFHEAD_MAGICNUM, strlen(ELFHEAD_MAGICNUM)))
	{
		printf("elf head magic num error.\n");
		return -1;
	}

	if (ehdr->e_type != ET_REL)
	{
		printf("module file is NOT a Relocatable file.\n");
		return -1;
	}

	if (ehdr->e_ehsize != sizeof(Elf32_Ehdr))
	{
		printf("elf head size error %d, should be sizeof(Elf32_Ehdr) = %d\n", ehdr->e_ehsize, sizeof(Elf32_Ehdr));
	}

	/* locate the string table which store all of the section name */
	mod->sect_idx.sect_name = ehdr->e_shstrndx;

	/* symbol resolve */
	if (resolve_symb(mod, ehdr, shdr) != 0)
		return -1;

	/* relocate */
	if (do_relocate(mod, ehdr, shdr) != 0)
		return -1;

	/* ok, we have done the symbol resolve and relocate */

	/* locate the SECTIONNAME_MODNAME section, get the module name */
	idx = findsectbyname(mod, SECTIONNAME_MODNAME, ehdr, shdr);
	if (idx < 0)
	{
		printf("can't find section \"%s\" in the module file.\n", SECTIONNAME_MODNAME);
		return -1;
	}
	mod->name = mod->modbuff + shdr[idx].sh_offset;

	/* locate the SECTIONNAME_MODFUNC section, get some mod functions */
	idx = findsectbyname(mod, SECTIONNAME_MODFUNC, ehdr, shdr);
	if (idx < 0)
	{
		printf("can't find section \"%s\" in the module file.\n", SECTIONNAME_MODFUNC);
		return -1;
	}
	modfunc_desc_t *modfunc = mod->modbuff + shdr[idx].sh_offset;
	while ((u32)modfunc < (u32)(mod->modbuff + shdr[idx].sh_offset + shdr[idx].sh_size))
	{
		switch (modfunc->functype)
		{
		case MODFUNC_INIT:
			mod->mod_init = (modfunc_init)(modfunc->funcaddr);
			break;
		case MODFUNC_UNINIT:
			mod->mod_uninit = (modfunc_uninit)(modfunc->funcaddr);
			break;
		}

		modfunc++;
	}

	return 0;
}

/* insmod filename */
static void cmd_insmod_opfunc(char *argv[], int argc, void *param)
{
	/* loader module file */
	if (argc != 2)
	{
		printf("usage: insmod filename.\n");
		return;
	}
	int filesize = fs_readfile(cursel_partition, argv[1], NULL);
	if (filesize < 0)
	{
		printf("Can't find module file \"%s\"\n", argv[1]);
		return;
	}
	DEBUG("module file size: %dM  %dK  %dB.\n",
		  (filesize >> PUBLIC_BITWIDTH_1M) & ((1 << PUBLIC_BITWIDTH_1K) - 1),
		  (filesize >> PUBLIC_BITWIDTH_1K) & ((1 << PUBLIC_BITWIDTH_1K) - 1),
		  filesize & ((1 << PUBLIC_BITWIDTH_1K) - 1));

	kmodule_t *mod = memcache_alloc(modcache);
	if (mod == NULL)
	{
		printf("alloc module desc faild.\n");
		return;
	}
	mod->size = filesize;
	mod->ocpy_rk = get_pagerank(filesize);

	/* alloc many continus pages to store module */
	mod->modbuff = page_alloc(mod->ocpy_rk, MMAREA_NORMAL);
	if (mod->modbuff == NULL)
		goto insmod_faild;

	/* real read mode file */
	ASSERT(fs_readfile(cursel_partition, argv[1], mod->modbuff) == filesize);

	if (0 != modfile_load(mod))
	{
		printf("insert module faild.\n");
		goto insmod_faild;
	}

	/* call the init func of this module */
	int ret = mod->mod_init();
	if (ret)
		printf("init func of module \"%s\" return %d.\n", mod->name, ret);

	/* insert into module list */
	list_add_head(&(mod->modlist), &kmodlist);

	return;

insmod_faild:
	if (mod->modbuff)
		page_free(mod->modbuff);

	memcache_free(modcache, mod);
}

struct command cmd_insmod _SECTION_(.array.cmd) =
{
    .cmd_name   = "insmod",
    .info       = "insert one module into mld.",
    .op_func    = cmd_insmod_opfunc,
};

static void cmd_lsmod_opfunc(char *argv[], int argc, void *param)
{
	kmodule_t *p;
	LIST_FOREACH_ELEMENT(p, &kmodlist, modlist)
	{
		printf("%s\t\t0x%#8x\t0x%#8x\n", p->name, p->size, 0x1 << (p->ocpy_rk + PAGESIZE_SHIFT));
	}
}

struct command cmd_lsmod _SECTION_(.array.cmd) =
{
    .cmd_name   = "lsmod",
    .info       = "list all modules in mld.",
    .op_func    = cmd_lsmod_opfunc,
};

static void cmd_rmmod_opfunc(char *argv[], int argc, void *param)
{
	u32 namelen = strlen(argv[1]);
	if (argc != 2)
	{
		printf("USAGE: rmmod modname\n");
		return;
	}

	kmodule_t *p;
	LIST_FOREACH_ELEMENT(p, &kmodlist, modlist)
	{
		if (namelen != strlen(p->name))
			continue;
		if (memcmp(argv[1], p->name, namelen) != 0)
			continue;

		/* call the module's uninit func */
		p->mod_uninit();

		list_remove(&(p->modlist));
		page_free(p->modbuff);
		memcache_free(modcache, p);

		printf("mod \"%s\" has been removed.\n", argv[1]);
		return;
	}

	printf("can't find mod \"%s\"\n", argv[1]);
}

struct command cmd_rmmod _SECTION_(.array.cmd) =
{
    .cmd_name   = "rmmod",
    .info       = "remove module.",
    .op_func    = cmd_rmmod_opfunc,
};

