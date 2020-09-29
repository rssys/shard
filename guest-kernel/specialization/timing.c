#include <linux/time.h>
#include "timing.h"

extern void do_gettimeofday(struct timeval *);

unsigned long long get_time(void) {
	struct timeval t;
	do_gettimeofday(&t);
	return t.tv_sec * 1000000 + t.tv_usec;
}
