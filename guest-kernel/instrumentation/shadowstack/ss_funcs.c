#include <string.h>
#include <stdio.h>

#define MAX_PROCS 50000
#define MAX_STACK_PER_PROCESS 200

struct proc_ss_info {
	int pid;
	int idx;
	long * shadow_stack_addr;
};

struct proc_ss_info ss_info[MAX_PROCS];
int ss_curr_pid = -1;
int num_procs = -1;

long shadow_stack_pool[MAX_STACK_PER_PROCESS * MAX_PROCS] = {1, 2, 3, 4, 5};
long * shadow_stack = shadow_stack_pool;
int shadow_stack_index = 5;

void __attribute__((noinline)) ss_entry(unsigned long * address) {
	__asm__("UD2\n\t");
	// Increment before update .. There can be an interrupt between increment and update.
	shadow_stack_index++;
	shadow_stack[shadow_stack_index - 1] = *address;
}

void __attribute__((noinline)) ss_exit(unsigned long *  address) {
	__asm__("UD2\n\t");
	// Decrement after checking .. There can be an interrupt between decrement and checking.
	if(shadow_stack[shadow_stack_index - 1] != *address) {
		// printf("shadow_stack is %lx address is %lx\n", shadow_stack[shadow_stack_index - 1], *address);
		__asm__("UD2\n\t");
	}
	shadow_stack_index--;
	if(shadow_stack_index == 0) __asm__("UD2\n\t");
}

int getIdx(int pid) {
	for(unsigned i = 0; i < num_procs; i++) {
		if(pid == ss_info[i].pid)
			return i;
	}
	return -1;
}

void save_old_ss() {
	int idx;
	if(num_procs < 0) num_procs = 0;
	if(ss_curr_pid < 0) return;
	idx = getIdx(ss_curr_pid);
	ss_info[idx].idx = shadow_stack_index;
	ss_curr_pid = -1;
}

void __attribute__((noinline)) add_new_ss(int next_pid, int tracked_proc) {
	int idx;
	if(num_procs < 0) num_procs = 0;
	if(!tracked_proc) return;
	idx = getIdx(next_pid);
	if(idx < 0) {
		idx = num_procs;
		num_procs++;
		ss_info[idx].pid = next_pid;
		ss_info[idx].idx = 0;
		ss_info[idx].shadow_stack_addr = shadow_stack_pool + idx * MAX_STACK_PER_PROCESS;
	}
	shadow_stack_index = ss_info[idx].idx;
	shadow_stack = ss_info[idx].shadow_stack_addr;
	ss_curr_pid = ss_info[idx].pid;
}