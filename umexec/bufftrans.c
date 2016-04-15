#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int fd, i;
	struct stat fst;
	void *mb;
	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		printf("can't open the file: %s", argv[1]);
		exit(0);
	}
	if (fstat(fd, &fst))
	{   
		close(fd);
		printf("Unable to stat %s'.", argv[1]);
		exit(0);
	}
	mb = mmap(NULL, fst.st_size, PROT_READ, MAP_SHARED, fd, 0);
	
	printf("const unsigned long userbin_len = 0x%x;\n\n", fst.st_size);
	
	printf("const char user_bin[] = {\n");
	for (i = 0; i < fst.st_size; i++)
	{
		printf("0x%x, ", *((unsigned char *)mb + i));
		if ((i & 0x7) == 0x7)
		{
			printf("\n");	
		}
	}
	
	printf("\n};\n");
	
	close(fd);
}
