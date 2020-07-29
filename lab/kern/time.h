#ifndef JOS_KERN_TIME_H
#define JOS_KERN_TIME_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

void time_init(void);
void time_tick(uint32_t cpunum);
unsigned int time_msec(uint32_t cpunum);

#endif /* JOS_KERN_TIME_H */
