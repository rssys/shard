#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cfi_lib.h"
#define text_section_offset 0xffffffff81000000
#define FRAME(addr) ((addr - text_section_offset)/4096)
#define OFFSET(addr) (addr & 0xFFF)

extern void * __kmalloc(size_t size, int flags);
extern int printk(const char *fmt, ...);
#define KERN_SOH	"\001"		/* ASCII Start Of Header */
#define KERN_INFO	KERN_SOH "6"

struct CallSite_to_Func {
	int id;
	void (*target)(void);
};

int num_callsite_targets;
struct CallSite_to_Func callsite_targets[num_call_site_targets];

bool * hash_tables[num_call_sites][num_ts_frames];
bool ht_initialized = false;

void initialize_ht(void) {
	unsigned i, j;
	for(i = 0; i < num_call_sites; i++) {
		for(j = 0; j < num_ts_frames; j++) {
			hash_tables[i][j] = NULL;
		}
	}
}

void __attribute__((noinline)) initialize_fe(int id, uint64_t frame) {
	int i;
	hash_tables[id][frame] = __kmalloc(4096, 21004480); // GFP_KERNEL
	for(i = 0; i < 4096; i++) {
		hash_tables[id][frame][i] = false;
	}
}

void __attribute__((noinline)) add_to_ht(int id, uint64_t func_addr) {
	uint64_t frame = FRAME(func_addr);
	uint64_t offset = OFFSET(func_addr);
	if(id == 289) {
		printk(KERN_INFO "------------------------------------> Initializing 289: %lx %lx %lx\n", func_addr, frame, offset);
		printk("**************************************************************");
	}
	if(id > num_call_sites) return;
	if(frame >= num_ts_frames) return;
	if(!hash_tables[id][frame]) initialize_fe(id, frame);
	hash_tables[id][frame][offset] = true;
}

void initialize_cs(void) {
	int i;
	for(i = 0; i < num_callsite_targets; i++) {
		add_to_ht(callsite_targets[i].id, (uint64_t) callsite_targets[i].target);
	}
}

void __attribute__((noinline)) initialize(void) {
	if(ht_initialized) return;
	initialize_ht();
	initialize_cs();
	ht_initialized = true; 
	__asm__("UD2\t\n");
}

uint64_t invalid_addr = 0xffffffffffffffff;
void __attribute__((noinline)) check_ci(int id, uint64_t func_addr) {
	uint64_t frame = FRAME(func_addr);
	uint64_t offset = OFFSET(func_addr);	
	if(frame >= num_ts_frames) return;
	if(!hash_tables[id][frame] || !hash_tables[id][frame][offset]) {
		invalid_addr = func_addr;
		__asm__("UD2\t\n");
		if(id == 289) {
			printk(KERN_INFO "For 289 %lx %lx %lx\n", func_addr, frame, offset);
			printk("**************************************************************");
		}
	}
}