#include <inc/lib.h>

// Waits until 'envid' exits and returns the retcode of the env
int
wait(envid_t envid)
{
	const volatile struct Env *e;

	assert(envid != 0);
	e = &envs[ENVX(envid)];
	while (e->env_id == envid && e->env_status != ENV_FREE)
		sys_yield();
	return e->env_retcode;
}
