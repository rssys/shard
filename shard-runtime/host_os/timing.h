#include <linux/time.h>

u64 get_time(void) {
	struct timeval t;
	do_gettimeofday(&t);
	return t.tv_sec * 1000000 + t.tv_usec;
}

// inline u64 get_time(void) {
// 	u64 val;
// 	__asm__ __volatile__("mfence":::"memory");
// 	val = rdtsc();
// 	__asm__ __volatile__("mfence":::"memory");
// 	return val;
// }

bool is_timing = false;

void start_timing(void) {
	is_timing = true;	
}

void stop_timing(void) {
	is_timing = false;
}

#define START_TIMER() \
	u64 start_t = 0; \
	if(is_timing) { \
	    start_t = get_time(); \
	}

#define END_TIMER(end_t) \
	if(is_timing && start_t) { \
	    end_t += (get_time() - start_t); \
	}

#define START_TIMER_WI(start_t) \
	if(is_timing) { \
	    start_t = get_time(); \
	}

#define END_TIMER_WI(start_t, end_t) \
	if(is_timing && start_t) { \
	    end_t += (get_time() - start_t); \
	}
