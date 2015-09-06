#include "public.h"

int mod_testvale;
int mod_testfunc(int i)
{
	mod_testvale = i;
	return (mod_testvale + 1);
}
