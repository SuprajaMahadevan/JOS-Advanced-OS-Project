
#include <inc/lib.h>

void
exit(int retcode)
{
	close_all();
	sys_env_destroy(0, retcode);
}

