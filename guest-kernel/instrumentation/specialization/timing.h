#include <linux/time.h>

extern unsigned long long get_time(void);

#define START_TIMER() \
	unsigned long long start_t = 0; \
    start_t = get_time();

#define END_TIMER(end_t) \
    if(end_t == 0xdeadbeef) end_t = 0; \
	end_t += (get_time() - start_t);
	