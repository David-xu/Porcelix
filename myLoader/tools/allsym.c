#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMTABLE_INCREASESZ			(0x1000)

typedef struct _symdesc {
	unsigned		addr;
	char			*name;
} symdesc_t;

int		n_sym = 0;
int		n_symcs = SYMTABLE_INCREASESZ;		/* cache size */
symdesc_t *symlist = NULL;

int read1sym(FILE *in, symdesc_t *sym)
{
	char str[256], type;			/* A t T R D ... */
	int rc;
	memset(str, 0, 256);
	rc = fscanf(in, "%x %c %255s\n", &(sym->addr), &type, str);
	if (rc != 3)
	{
		fscanf(in, "%c %255s\n", &type, str);
		sym->addr = 0;
	}

	sym->name = (char *)malloc(strlen(str) + 2);
	sym->name[0] = type;
	strcpy(&(sym->name[1]), str);
	sym->name[strlen(str) + 1] = 0;

	return 0;
}

static int readsyms(FILE *in)
{
	while (!feof(in))
	{
		if (n_sym >= n_symcs)
		{
			n_symcs += SYMTABLE_INCREASESZ;
			symlist = (symdesc_t *)realloc(symlist, sizeof(symdesc_t) * n_symcs);
		}
		read1sym(in, &(symlist[n_sym]));
		n_sym++;
	}

	return 0;
}

/*
 * withsnf: with symbol name label
 *          1 ==> the product asm will contain label of each symbol name buf
                  this is used by module export symbol
 */
static int write_asm(int withsnlb)
{
	int i, len, totallen = 0;
	/*  */
	printf(".section .rodata\n");
	printf(".global n_allsym\n"
		   "n_allsym:\n"
		   ".long %d\n", n_sym);

	printf(".global allsym_addr\n" "allsym_addr:\n");
	for (i = 0; i < n_sym; i++)
	{
		printf(".long %d\n", symlist[i].addr);
	}

	printf(".global allsym_namebuff\n" "allsym_namebuff:\n");
	for (i = 0; i < n_sym; i++)
	{
		len = strlen(symlist[i].name);
		
		totallen += len + 1;
		if (withsnlb)
			printf("expsymname_%s:\n", &(symlist[i].name[1]));
		printf(".ascii	\"%s\\0\"\n", symlist[i].name);
	}
	
	printf(".global sz_allsymnamebuff\n"
		   "sz_allsymnamebuff:\n"
		   ".long %d\n", totallen);

	return 0;
}

/* read the nm -n core.elf from stdin */
/*
 * -snl: symbol name label
 */
int main(int argc, char *argv[])
{
	int i, snl = 0;
	symlist = (symdesc_t *)malloc(sizeof(symdesc_t) * n_symcs);

	readsyms(stdin);
	if (argc == 2)
		if (memcmp("-snl", argv[1], strlen("-snl")) == 0)
			snl = 1;

	write_asm(snl);

	for (i = 0; i < n_sym; i++)
	{
		free(symlist[i].name);
	}
	free(symlist);

	return 0;
}
