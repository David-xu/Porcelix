#include "public.h"
#include "module.h"
#include "io.h"
#include "memory.h"

static u32 count;
static memcache_t *testcache;
module_name("testmod");

extern int mod_testvale;
extern int mod_testfunc();

static int testmod_init(void)
{
	printk("testmod init.\n");

	for (count = 0; count < mem_resource.n_range; count++)
	{
		printk("[0x%#8x : 0x%#8x)  0x%#8x\n",
			   mem_resource.rd[count].begin,
			   mem_resource.rd[count].begin + mem_resource.rd[count].size,
			   mem_resource.rd[count].flag);
	}
	testcache = memcache_create(4, 0, "testmod cache");
	memcache_alloc(testcache);

	mod_testfunc(2);
	
	return 0;
}

static void testmod_uninit(void)
{
	memcache_destroy(testcache);
	printk("testmod uninit. %d %d\n", count, mod_testvale);
}

module_init(testmod_init, 7);
module_uninit(testmod_uninit);

