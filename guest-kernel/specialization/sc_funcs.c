#include <string.h>

#define true 1
#define false 0

unsigned long long syscall_entry_time = 0xdeadbeef;
unsigned long long syscall_exit_time = 0xdeadbeef;
unsigned long long ctx_switch_time = 0xdeadbeef;


char tracked_procs[1000] = {"lighttpd"};
char procs_to_skip[1000] = {"init"};
int num_tracked_procs = -1;
int num_procs_to_skip = -1;

int num_irqs = 0;
int in_exc = false;



/* Used for tracking context switches to tracked processes */

int is_tracked_proc(char * proc_name) {
	int i;
	for(i = 0; i < num_procs_to_skip; i++) {
		if(!strncmp(procs_to_skip + i * 100, proc_name, strlen(procs_to_skip + i * 100))) {
			return 0;
		}		
	}
	for(i = 0; i < num_tracked_procs; i++) {
		if(!strncmp(tracked_procs + i * 100, proc_name, strlen(tracked_procs + i * 100))) {
			return 1;
		}
	}
	return 0;
}

int currently_running = false;
int curr_pid = -1;
char curr_prname[100] = {"helloworld\0"};
void __attribute__((noinline)) ud2_call(int pid, int is_tracked) {
	curr_pid = pid;
	currently_running = is_tracked;
	__asm__("ud2");	
}

void set_tracking(int pid, char ** parents_array, int num) {
	int len = strlen(parents_array[0]);
	memcpy(curr_prname, parents_array[0], len);
	curr_prname[len] = '\0';
	int is_tracked = false;
	for(int i = 0; i < num; i++) {
		is_tracked = is_tracked_proc(parents_array[i]);
		if(is_tracked) break;
	}
	if(is_tracked || currently_running) ud2_call(pid, is_tracked);
}

long curr_sc_id = -1;


/* Used for tracking system calls */
void __attribute__((noinline)) sc_entry(long sc_id) {
	if(!currently_running) return;
	in_exc = false;
	curr_sc_id = sc_id;
	// START_TIMER();
	__asm__("ud2\n\t");
	// END_TIMER(syscall_entry_time);
}

void __attribute__((noinline))  sc_exit(long sc_id) {
	if(!currently_running) return;
	in_exc = true;
	curr_sc_id = sc_id;
	// START_TIMER();
	__asm__("ud2\n\t");
	// END_TIMER(syscall_exit_time);
}

/* Used for tracking interrupts */
void __attribute__((noinline)) log_irq_enter(void) {
    num_irqs++;
	__asm__("NOP\n\t");
	__asm__("NOP\n\t");	
}

void __attribute__((noinline)) log_irq_exit(void) {
    if(num_irqs) num_irqs--;
	__asm__("NOP\n\t");	
	__asm__("NOP\n\t");
}


/* Used for Profiling */
typedef char bool;
bool system_call_to_func_mapping[334][20000] = {{0, 1, 2, 3, 4, 5}};
bool interrupt_view[20000] = {0, 1, 2, 3, 4, 5};
bool exception_view[20000] = {0, 1, 2, 3, 4, 5};
bool initialized = false;

char curr_fn_name[100] = "helloworld";

int are_we_tracking = -1;

void __attribute__((noinline)) log_fn(int id, char * str, int size) {
	if(are_we_tracking <= 0) return;
	if(!initialized) {
		for(int j = 0; j < 20000; j++) {
			interrupt_view[j] = false;
			exception_view[j] = false;
			for(int i = 0; i < 334; i++) {
				system_call_to_func_mapping[i][j] = false;
			}
		}
		initialized = true;
	}
	// printk("function is %s num_irqs %d, in_exc %d, id %d\n", curr_sc_id);
    if(num_irqs) {
    	if(interrupt_view[id]) return;
    	interrupt_view[id] = true;
    } else if(in_exc) {
    	if(exception_view[id]) return;
    	exception_view[id] = true;
    } else {
    	if(system_call_to_func_mapping[curr_sc_id][id]) return;
    	system_call_to_func_mapping[curr_sc_id][id] = true;
    }
	memcpy(curr_fn_name, str, size);
	curr_fn_name[size] = '\0';
	__asm__("nop\n\t");
	__asm__("nop\n\t");
}