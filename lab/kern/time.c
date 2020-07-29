#include <kern/time.h>
#include <kern/cpu.h>

#include <inc/assert.h>

// Maintain number of ticks for each CPU. Use CPU num
// to index array for number of ticks for that CPU.
static unsigned int ticks[NCPU];

void
time_init(void)
{
	uint32_t i;
	for (i = 0; i < NCPU; i++)
		ticks[i] = 0;
}

// This should be called once per timer interrupt.  A timer interrupt
// fires every 10 ms.
void
time_tick(uint32_t cpunum)
{
	ticks[cpunum]++;
	if (ticks[cpunum] * 10 < ticks[cpunum])
		panic("time_tick: time overflowed");
}

unsigned int
time_msec(uint32_t cpunum)
{
	return ticks[cpunum] * 10;
}
