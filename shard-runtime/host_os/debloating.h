#ifndef KVM_UNIFDEF_H
#define KVM_UNIFDEF_H

#ifdef __i386__
#ifndef CONFIG_X86_32
#define CONFIG_X86_32 1
#endif
#endif

#ifdef __x86_64__
#ifndef CONFIG_X86_64
#define CONFIG_X86_64 1
#endif
#endif

#if defined(__i386__) || defined (__x86_64__)
#ifndef CONFIG_X86
#define CONFIG_X86 1
#endif
#endif

#ifdef __PPC__
#ifndef CONFIG_PPC
#define CONFIG_PPC 1
#endif
#endif

#ifdef __s390__
#ifndef CONFIG_S390
#define CONFIG_S390 1
#endif
#endif

#endif

#include <linux/kvm_host.h>
#include <asm/vmx.h>
#include <asm/traps.h>
#include "x86.h"
#include "mmu.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <asm/mman.h>

#include "timing.h"
#include "printing.h"
#include "fileio.h"
#include "guest_addrs.h"
#include "numbers.h"
#include "lbr.h"
#include  "syscallinfo.h"
#include "shard_work_dir.h"

#define text_section_offset 0xffffffff81000000
#define text_section_size 0x800000
#define num_sys_calls 334
#define FRAME(addr) ((addr - text_section_offset)/4096) 
#define OFFSET(addr) (addr & 0xFFF)
bool HAS_HANDLED = false;
char ud2_buffer[] = {0xf,0xb};
char nop_buffer[1] = {0x90};
struct file * fd_log = NULL;

char dt_fpath_is[250];
char dt_fpath_qemu_addrs[250];
char dt_fpath_kvm_log[250];
char dt_fpath_frame_info[250];
char dt_fpath_cfg[250];

/* guest kernel function information */
void initialize_kernel_path(char * path_arr, char * fname) {
    int len1 = strlen(SHARD_WORK_DIR);
    int len2 = strlen(fname);
    dt_printk("%d %s %d %s\n", len1, SHARD_WORK_DIR, len2, fname);

    memcpy(path_arr, SHARD_WORK_DIR, len1);
    path_arr[len1] = '/';
    memcpy(&path_arr[len1 + 1], fname, len2);
    path_arr[len1 + 1 + len2] = '\0';
    dt_printk("%s\n", path_arr);
}

void initialize_kernel_paths(void) {
    dt_printk("called initialize kernel paths\n");
    initialize_kernel_path(dt_fpath_is, "dt_func_info_is");
    initialize_kernel_path(dt_fpath_qemu_addrs, "qemu_addrs");
    initialize_kernel_path(dt_fpath_kvm_log, "kvm_log");
    initialize_kernel_path(dt_fpath_frame_info, "sc_info");
    initialize_kernel_path(dt_fpath_cfg, "dt_cfg");
}
/* guest kernel function information */

#define MAX_GUEST_FUNCS 25000
struct func_node {
    char * name;
    unsigned long address;
    int code_size;
    unsigned long * targets;
    int num_targets;
};

struct func_node ** fn_array;
int fn_array_size = 0;

/* GUEST kernel state information */
enum state {
    ORIGINAL,
    HARDENED,
    UD2
};

enum state curr_state = ORIGINAL;

/* Guest process information */

struct proc_info {
    int pid;
    bool is_set;
    enum state curr_state;
    int curr_context;
    bool is_hardened;
    unsigned long ret_addr;
    char ret_buf[2];
};

#define MAX_PID 50000
struct proc_info processes[MAX_PID];

char curr_proc_name[100] = "helloworld\0";
int curr_pid;
int tracking_enabled = false;

char * tracked_proc_list[] = {
    "runspec\0",
    // "hmmer_base.ia64\0",
    // "bzip2_base.ia64\0"
    // "mcf_base.ia64\0",
    // "dhry2reg\0",
    // "whetstone-doubl\0",
    // "execl\0",
    // "fstime\0",
    // "pipe\0",
    // "context1\0",
    // "spawn\0",
    // "looper\0",
    // "multi.sh\0",
    // "syscall\0"
    // "exploit\0",
    // "brotli\0",
    // "udp_server\0",
    // "redis-server\0",
    // "redis-benchmark\0",
    "nginx\0",
    "sysinfo\0",
    "lighttpd\0",
    "syscall\0"
};

/* Qemu writes these variables to a file - only text_addrs are needed */
unsigned long ss_text_addr, oos_text_addr, original_text_addr, ud2_text_addr;


/* At time of booting original kernel text section may be modified as a result
after booting we need to update relevant code in ss and ud2 kernels.
*/
struct code_loc {
    unsigned long address;
    int size;
};

struct code_loc changes_between_contexts[100000];
int num_changes_between_contexts = 0;
bool first_time_ctx_handling = true;

/* For each system call frame information */

struct SyscallInfo syscall_info[num_sys_calls];

/* 
    current system call executing on the guest - only relevant if the current guest process is tracked 
*/
int syscall_id = 0;

/* This information is maintained while profiling to prevent redundancy in profile */
int num_irqs = 0;
bool in_exc = false;
bool in_syscall = false;

#ifdef LARGE_FILE
#define MAX_FUNCS_PER_CONTEXT 3000
char * syscall_funcs[num_sys_calls][MAX_FUNCS_PER_CONTEXT];
int syscall_num_funcs[num_sys_calls];
bool syscall_funcs_initialized = false;
char * global_funcs[MAX_FUNCS_PER_CONTEXT];
int global_num_funcs = 0;
#endif

unsigned long dt_get_gpa(struct kvm_vcpu *vcpu, unsigned long addr) {
    struct x86_exception e;
    return vcpu->arch.walk_mmu->gva_to_gpa(vcpu, addr, 0, &e);    
}

unsigned long dt_get_gfn(struct kvm_vcpu *vcpu, unsigned long addr) {
    unsigned long gfn;
    gpa_t gpaddr;
    struct x86_exception e;
    gpaddr = vcpu->arch.walk_mmu->gva_to_gpa(vcpu, addr, 0, &e);
    gfn = gpaddr >> PAGE_SHIFT;
    return gfn;   
}

unsigned long dt_get_hva(struct kvm_vcpu *vcpu, unsigned long addr) {
    unsigned long hva;
    gpa_t gpaddr;
    struct x86_exception e;
    gpaddr = vcpu->arch.walk_mmu->gva_to_gpa(vcpu, addr, 0, &e);
    hva = gfn_to_hva(vcpu->kvm, gpaddr >> PAGE_SHIFT);
    hva += gpaddr & (~PAGE_MASK);
    return hva;
}
 
struct kvm_memory_slot * dt_get_memslot(struct kvm_vcpu *vcpu, unsigned long addr) {
    struct x86_exception e;
    gpa_t gpaddr = vcpu->arch.walk_mmu->gva_to_gpa(vcpu, addr, 0, &e);
    return gfn_to_memslot(vcpu->kvm, gpaddr >> PAGE_SHIFT); 
}

unsigned long dt_get_stack_addr(struct kvm_vcpu * vcpu, unsigned long stackp) {
    char buffer[8];
    int ptr_size = sizeof(unsigned long);
    copy_from_user(buffer, (void *) dt_get_hva(vcpu, stackp), ptr_size);  
    return *(long *) &buffer;
}

bool is_return_at_addr(struct kvm_vcpu * vcpu, unsigned long instp) {
    char byte;
    copy_from_user(&byte, (void *) dt_get_hva(vcpu, instp), 1);
    return byte == 0xc3;
}

unsigned long get_transfer_from(unsigned long to) {
    return get_lbr_from(to);
}

void set_curr_proc_name(struct kvm_vcpu * vcpu) {
    if(is_timing) return;
    copy_from_user(curr_proc_name, (void *) dt_get_hva(vcpu, curr_proc_name_addr), 100);
    if(!strncmp("ab", curr_proc_name, strlen("ab"))) {
        start_timing();
    }
}

void print_guest_tracked_procs(struct kvm_vcpu * vcpu) {
    char buffer[100];
    int i, num_tracked_procs;
    copy_from_user(&num_tracked_procs, (void *) dt_get_hva(vcpu, num_tracked_procs_addr), 4);
    dt_printk("num_tracked_procs is %d\n", num_tracked_procs);    
    for(i = 0; i < 10; i++) {
        copy_from_user(buffer, (void *) dt_get_hva(vcpu, tracked_procs_addr) + i * 100, 100);
        dt_printk("%d. %s\n", i, buffer);
    }
}

void set_guest_tracked_procs(struct kvm_vcpu * vcpu) {
    int num_tracked_procs = sizeof(tracked_proc_list)/sizeof(char *), i;
    dt_printk("num_tracked_procs is %d\n", num_tracked_procs);
    copy_to_user((void *) dt_get_hva(vcpu, num_tracked_procs_addr), &num_tracked_procs, 4);
    for(i = 0; i < num_tracked_procs; i++) {
        dt_printk("adding process %s\n", tracked_proc_list[i]);
        copy_to_user((void *) dt_get_hva(vcpu, tracked_procs_addr) + i * 100, tracked_proc_list[i], strlen(tracked_proc_list[i]) + 1);
    }
}

bool is_tracked_proc(struct kvm_vcpu * vcpu) {
    int i;
    write_to_log_sprintf(fd_log, "next process is %s\n", curr_proc_name);
    for(i = 0; i < sizeof(tracked_proc_list)/sizeof(char *); i++) {
        if(!strncmp(tracked_proc_list[i], curr_proc_name, strlen(tracked_proc_list[i]))) {
            return true;
        }
    }
    return false;
}

void set_curr_proc_info(struct kvm_vcpu * vcpu) {
    curr_pid = kvm_register_read(vcpu, VCPU_REGS_RDI);    
    tracking_enabled = kvm_register_read(vcpu, VCPU_REGS_RSI);
}

struct func_node * dt_init_func_node(char * name, unsigned long address, int code_size) {
    struct func_node * fn = kmalloc(sizeof(struct func_node), GFP_KERNEL);
    if(!fn) {
        dt_printk("failed to initialize func node\n");
        return NULL;
    }
    fn->name = name;
    fn->address = address;
    fn->code_size = code_size;
    fn->targets = NULL;
    fn->num_targets = 0;
    return fn;
}

struct func_node * dt_get_fnode(unsigned long address, struct func_node ** fn_array, unsigned lo, unsigned hi) {
    unsigned mid; struct func_node * fn;
    if(lo == hi) return NULL;
    mid = lo + (hi - lo)/2;
    fn = fn_array[mid];
    if(address >= fn->address && address < (fn->address + fn->code_size)) return fn;
    if(address < fn->address) return dt_get_fnode(address, fn_array, lo, mid);
    else return dt_get_fnode(address, fn_array, mid + 1, hi);
}

struct func_node * dt_get_fnode_name(char * fname, struct func_node ** fn_array, unsigned lo, unsigned hi) {
    unsigned i;
    struct func_node * fn;
    for(i = lo; i < hi; i++) {
        fn = fn_array[i];
        if(!strcmp(fname, fn->name)) {
            return fn;
        }
    }
    return NULL;
}

bool check_transfer(struct kvm_vcpu * vcpu, unsigned long caller, unsigned long callee) {
    int i;
    struct func_node * fn; 
    if(is_return_at_addr(vcpu, caller)) return false;
    fn = dt_get_fnode(caller, fn_array, 0, fn_array_size);
    if(!fn) return true; // we cannot handle these functions
    for(i = 0; i < fn->num_targets; i++) {
        if(fn->targets[i] == callee) return true;
    }
    return false;
}

struct func_node * dt_parse_token(struct file * fd) {
    char buffer[8];
    int namelen, code_size, inst_size1, inst_size2;
    char * name, * code;
    unsigned long address;
    if(!dt_do_safe_read(fd, buffer, 4)) {
        return NULL;
    }
    namelen = *(int *) buffer;
    name = kmalloc(sizeof(char) * (namelen + 1), GFP_KERNEL);
    if(!name) {
        dt_printk("dt_parse_token : failed to allocate name\n");
        return NULL;
    }
    if(!dt_do_safe_read(fd, name, namelen)) {
        return NULL;
    }
    name[namelen] = '\0';
    if(!dt_do_safe_read(fd, buffer, 8)) {
        return NULL;
    }
    address = *(long *) buffer;
    if(!dt_do_safe_read(fd, buffer, 4)) {
        return NULL;
    }
    code_size = *(int *) buffer;

    if(!dt_do_safe_read(fd, (char *) &inst_size1, 4)) {
        return NULL;
    }
    if(!dt_do_safe_read(fd, (char *) &inst_size2, 4)) {
        return NULL;
    }
    write_to_log_sprintf(fd_log, "dt_parse_token : namelen -> %d, name -> %s address -> 0x%lx code_size -> %d, inst_size1 %d, inst_size2 %d\n", namelen, name, address, code_size, inst_size1, inst_size2);
    code = kmalloc(sizeof(char) * code_size, GFP_KERNEL);
    if(!code) {
        dt_printk("dt_parse_token : failed to allocate code\n");
        return NULL;
    }
    if(!dt_do_safe_read(fd, code, code_size)) {
        return NULL;    
    }
    kfree(code);
    return dt_init_func_node(name, address, code_size);
}

bool is_white_listed(char * name) {
    int i;
    #define num 7
    char white_list[num][100] = {"rcu_bh_qs\0", "grab_cache_page_write_begin\0", "rcu_sched_qs\0", "ptep_clear_flush\0", "sg_free_table_chained\0", "anon_vma_ctor\0", "scsi_put_command\0"};
    for(i = 0; i < num; i++) {
        if(!strcmp(name, white_list[i]))
            return true;
    }
    return false;
}

int dt_load_mem(struct kvm_vcpu * vcpu, char * fname, struct func_node ** fn_array) {
    struct file * fd;
    struct func_node * fn;
    int fn_array_size = 0;
    fd = filp_open(fname, O_RDONLY, 0);
    dt_printk("reading file  %s\n", fname);

    if(fd < 0) {
        dt_printk("dt_load_mem : failed to open file %s\n", fname);
        return fn_array_size;
    }
    while(true) {
        fn = dt_parse_token(fd);
        if(!fn) break;
        if(fn_array_size >= MAX_GUEST_FUNCS) {
            dt_printk("--------------------------------> size is greater than MAX_GUEST_FUNCS\n");
            return fn_array_size;
        }
        if(is_white_listed(fn->name)) {
            continue;
        }
        fn_array[fn_array_size] = fn;
        fn_array_size++;
        changes_between_contexts[num_changes_between_contexts].address = fn->address;
        changes_between_contexts[num_changes_between_contexts].size = fn->code_size;        
        num_changes_between_contexts++;        
    }
    file_close(fd);
    return fn_array_size;
}

void dt_load_hva_addrs(void) {
    int temp;
    struct file * fd = filp_open(dt_fpath_qemu_addrs, O_RDONLY, 0);
    if(fd < 0) {
        dt_printk("dt_load_hva_addrs : failed to open file %s\n", dt_fpath_qemu_addrs);
        return;
    }
    /* read 4 integers */
    if(!dt_do_safe_read(fd, (char *) &temp, 4)) {
        return;
    }
    if(!dt_do_safe_read(fd, (char *) &temp, 4)) {
        return;
    }
    if(!dt_do_safe_read(fd, (char *) &temp, 4)) {
        return;
    }
    if(!dt_do_safe_read(fd, (char *) &temp, 4)) {
        return;
    }
    
    if(!dt_do_safe_read(fd, (char *) &ss_text_addr, 8)) {
        return;
    }
    if(!dt_do_safe_read(fd, (char *) &oos_text_addr, 8)) {
        return;
    }
    if(!dt_do_safe_read(fd, (char *) &original_text_addr, 8)) {
        return;
    }
    if(!dt_do_safe_read(fd, (char *) &ud2_text_addr, 8)) {
        return;
    }
}

bool dt_parse_cfg_function(struct file * fd) {
    int size, i, num_targets;
    unsigned long address, * targets;
    struct func_node * fn;
    
    if(!dt_do_safe_read(fd, (char *) &address, 8)) {
        dt_printk("completed cfg parsing\n");
        return false;
    }

    if(!dt_do_safe_read(fd, (char *) &size, 4)) {
        dt_printk("error while parsing size\n");
        return false;
    }
    targets = kmalloc(sizeof(unsigned long) * size, GFP_KERNEL);
    if(!targets) {
        dt_printk("dt_parse_cfg : failed to allocate targets\n");
        return false;
    }
    num_targets = 0;
    for(i = 0; i < size; i++) {
        if(!dt_do_safe_read(fd, (char *) &targets[num_targets], 8)) {
            dt_printk("error while parsing target number %d\n", i);
            return false;
        }
        fn = dt_get_fnode(targets[num_targets], fn_array, 0, fn_array_size);
        // if(fn && !strcmp(fn->name, "get_root_fork")) {
        //     dt_printk("skipping get_root_fork\n");
        //     continue;
        // }
        num_targets++;
    }
    fn = dt_get_fnode(address, fn_array, 0, fn_array_size);
    if(!fn) {
        return true;
    }
    fn->targets = targets;
    fn->num_targets = num_targets;
    return true;
}

void dt_parse_cfg(void) {
    struct file * fd = filp_open(dt_fpath_cfg, O_RDONLY, 0);
    dt_printk("parsing file %s\n", dt_fpath_cfg);
    if(!fd) {
        dt_printk("failed to open file\n");
        return;
    }
    while(dt_parse_cfg_function(fd)) {}
    file_close(fd);
}

void dt_initialize(struct kvm_vcpu * vcpu) {
    dt_printk("*********************************** INITIALIZING ***********************************");
    fn_array = kmalloc(sizeof(struct func_node) * MAX_GUEST_FUNCS, GFP_KERNEL);
    if(!fn_array) {
        printk("dt_initialize : failed to allocate fn_array\n");
        return;
    }
    fn_array_size = dt_load_mem(vcpu, dt_fpath_is, fn_array);

    dt_load_hva_addrs();
    fd_log = dt_init_log(dt_fpath_kvm_log);
    dt_set_mmu_log(fd_log);
    dt_parse_frame_info(dt_fpath_frame_info);
    dt_parse_cfg();
    dt_printk("num_changes_between_contexts %d\n", num_changes_between_contexts);
}
// #ifdef STOP_CHANGING_SCOPE
// bool changed_scope_once = false;
// #endif

void dt_change_scope(struct kvm_vcpu * vcpu, int kernel_state, int context) {
    #ifndef STOP_CHANGING_SCOPE
    #ifdef TIME_IT
    START_TIMER();
    #endif
    #endif
    if(kernel_state > 2) {
        dt_printk("won't change pfns .. state is %d\n", kernel_state);
        return;
    }
    #ifdef STOP_CHANGING_SCOPE
    return;
    // if(changed_scope_once) return;
    // changed_scope_once = true;
    #endif
    write_to_log_sprintf(fd_log, "Changing Scope\n");
    if(!dt_change_pfns_2(vcpu, kernel_state, context)) { // false means same state
        write_to_log_sprintf(fd_log, "same state\n");
        return;
    }
    #ifdef TIME_IT
    END_TIMER(usa_time);
    #endif
    if(in_ctx) {
        num_ctx_changes_in_scope++;
    } else if(in_sc) {
        num_sc_changes_in_scope++;
    }
    write_to_log_sprintf(fd_log, "changed state to %d kernel_state, %d context\n", kernel_state, context);
}

bool testing = false;

void ts_write(unsigned long address, int state, char * buffer, int size) {
    unsigned long frame_addr;
    int copy_size;
    int offset_in_page;
    if(testing) {
        write_to_log_sprintf(fd_log, "testing %x %x\n", buffer[0], buffer[1]);
    }
    if(FRAME(address) < 0 || FRAME(address) >= num_ts_frames) return;
    while(size > 0) {
        offset_in_page = address & (~PAGE_MASK);
        frame_addr = dt_get_frame(state, FRAME(address));
        copy_size = min((int) (PAGE_SIZE - offset_in_page), size);
        if(testing) {
            write_to_log_sprintf(fd_log, "%lx %d %lx\n", address, offset_in_page, frame_addr);
        }
        memcpy((char *) frame_addr + offset_in_page, buffer, copy_size);
        size -= copy_size;
        address += copy_size;
        buffer += copy_size;
    } 
    testing = false;
}

void ts_read(unsigned long address, int state, char * buffer, int size) {
    unsigned long frame_addr;
    int copy_size;
    int offset_in_page;
    if(FRAME(address) < 0 || FRAME(address) >= num_ts_frames) return;
    while(size > 0) {
        offset_in_page = address & (~PAGE_MASK);
        frame_addr = dt_get_frame(state, FRAME(address));
        copy_size = min((int) (PAGE_SIZE - offset_in_page), size);
        memcpy(buffer, (char *) frame_addr + offset_in_page, copy_size);
        size -= copy_size;
        address += copy_size;
        buffer += copy_size;
    }
}

void nop_to_ud2(struct kvm_vcpu * vcpu, unsigned long addr, int state) {
    if(addr >= text_section_offset  && addr < text_section_offset + text_section_size) {
        ts_write(addr, state, ud2_buffer, 2);
    }
    if(addr >= text_section_offset || state == ORIGINAL) {
        copy_to_user((char *) dt_get_hva(vcpu, addr), ud2_buffer, 2);
    }    
}

void ud2_to_nop(struct kvm_vcpu * vcpu, unsigned long addr, int state) {
    if(addr >= text_section_offset  && addr < text_section_offset + text_section_size) {
        ts_write(addr, state, nop_buffer, 1);
        ts_write(addr + 1, state, nop_buffer, 1);
    }
    if(addr >= text_section_offset || state == ORIGINAL) {
        copy_to_user((char *) dt_get_hva(vcpu, addr), nop_buffer, 1);
        copy_to_user((char *) dt_get_hva(vcpu, addr + 1), nop_buffer, 1);
    }
}

int print_status(struct kvm_vcpu * vcpu, unsigned long addr, int state) {
    char buf[2];
    if(addr >= text_section_offset  && addr < text_section_offset + text_section_size) {
        ts_read(addr, state, buf, 2);
    }
    if(addr >= text_section_offset || state == ORIGINAL) {
        copy_from_user(buf, (char *) dt_get_hva(vcpu, addr), 2);
    }
    if(buf[0] == ud2_buffer[0] && buf[1] == ud2_buffer[1]) return 0;
    if(buf[0] == nop_buffer[0] && buf[1] == nop_buffer[0]) return 1;
    return 2;
}

/* Start  logging guest.
Overwrite nop bytes of log_fn with ud2
Start tracking irq_enter and irq_exit
*/
void start_logging(struct kvm_vcpu * vcpu, int state) {
    int awt = 1;
    write_to_log_sprintf(fd_log, "start_logging\n");
    nop_to_ud2(vcpu, log_fn_addr, state);
    nop_to_ud2(vcpu, log_irq_enter_addr, state);
    nop_to_ud2(vcpu, log_irq_exit_addr, state);
    copy_to_user((char *) dt_get_hva(vcpu, are_we_tracking_addr), &awt, 4);
}


/* Stop  logging guest.
Overwrite ud2 bytes of log_fn with nop
Stop tracking irq_enter and irq_exit
*/
void stop_logging(struct kvm_vcpu * vcpu, int state) {
    int awt = 0;
    write_to_log_sprintf(fd_log, "stop_logging\n");
    ud2_to_nop(vcpu, log_fn_addr, state);
    ud2_to_nop(vcpu, log_irq_enter_addr, state);
    ud2_to_nop(vcpu, log_irq_exit_addr, state);
    copy_to_user((char *) dt_get_hva(vcpu, are_we_tracking_addr), &awt, 4);
}

bool lies_in_frame(unsigned long frame_addr, unsigned long address, int size, int * start, int * end) {
    if(address < frame_addr + PAGE_SIZE && address + size > frame_addr) {
        *start = address >= frame_addr ? address - frame_addr : 0;
        *end = address + size <= frame_addr + PAGE_SIZE ? address + size - frame_addr : PAGE_SIZE;
        return true;
    }
    return false;
}

bool finishes_in_or_before_frame(unsigned long frame_addr, unsigned long address, int size) {
    return address + size < frame_addr + PAGE_SIZE;
}

/* When the guest kernel boots, it modifies its own text section. Consequently, the original text section has
been modified but the hardened and overwritten text sections have not. In this function we will update the hardened
and overwritten kernels */

void handle_first_time_ctx_handling(struct kvm_vcpu * vcpu) {
    int j, start, end;
    unsigned long frame_offset;
    unsigned long undo_copy_offset;
    int undo_copy_size, curr_idx = 0;
    struct code_loc curr;
    char buffer[PAGE_SIZE];
    char old_buffer1[PAGE_SIZE];
    char old_buffer2[PAGE_SIZE];
    struct func_node * fn;
    write_to_log_sprintf(fd_log, "handle_first_time_ctx_handling %d *************************************\n", num_changes_between_contexts);
    
    for(j = 0; j < num_ts_frames; j++) {
        frame_offset = j << PAGE_SHIFT;
        copy_from_user(buffer, (char *) dt_get_hva(vcpu, text_section_offset + frame_offset), PAGE_SIZE);
        copy_from_user(old_buffer1, (char *) ss_text_addr + frame_offset, PAGE_SIZE);
        copy_from_user(old_buffer2, (char *) ud2_text_addr + frame_offset, PAGE_SIZE);
        copy_to_user((char *) ss_text_addr + frame_offset, buffer, PAGE_SIZE);
        copy_to_user((char *) ud2_text_addr + frame_offset, buffer, PAGE_SIZE);
        while(true) {
            curr = changes_between_contexts[curr_idx];
            if(lies_in_frame(text_section_offset + frame_offset, curr.address, curr.size, &start, &end)) {
                undo_copy_offset = frame_offset + start;
                undo_copy_size = end - start;
                copy_to_user((char *) ss_text_addr + undo_copy_offset, old_buffer1 + start, undo_copy_size);
                copy_to_user((char *) ud2_text_addr + undo_copy_offset, old_buffer2 + start, undo_copy_size);
                // dt_printk("undo copy at address %lx, size %d", text_section_offset + frame_offset + start, undo_copy_size);
            }
            
            if(finishes_in_or_before_frame(text_section_offset + frame_offset, curr.address, curr.size)) {
                curr_idx++;
            } else {
                break;
            }
            if(curr_idx >= num_changes_between_contexts)
                break;
        }
    }
    write_to_log_sprintf(fd_log, "considered %d changes_between_contexts\n", curr_idx);

    for(j = 0; j < fn_array_size; j++) {
        // copy_from_user(buffer, (char *) ud2_text_addr + (fn_array[j]->address - text_section_offset), 2);
        // if(buffer[0] == nop_buffer[0] && buffer[1] == nop_buffer[0]) {
        //     copy_to_user((char *) ud2_text_addr + (fn_array[j]->address - text_section_offset), ud2_buffer, 2);
        // }
        copy_to_user((char *) ud2_text_addr + (fn_array[j]->address - text_section_offset), ud2_buffer, 2);
    }

    // remove ud2 from get_random_bytes

    fn = dt_get_fnode_name("get_random_bytes", fn_array, 0, fn_array_size);
    if(!fn) {
        write_to_log_sprintf(fd_log, "failed to find get_random_bytes\n");
    } else {
        copy_from_user(buffer, (char *) original_text_addr + (fn->address - text_section_offset), fn->code_size);
        copy_to_user((char *) ss_text_addr + (fn->address - text_section_offset),  buffer, fn->code_size);
    }

    for(j = 0; j < 2; j++) {
        copy_to_user((char *) original_text_addr + (log_irq_enter_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (log_irq_enter_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (log_irq_enter_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) original_text_addr + (log_irq_exit_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (log_irq_exit_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (log_irq_exit_addr - text_section_offset) + j, nop_buffer, 1);
    }


    #ifdef DISABLE_SC_ENTRY
    for(j = 0; j < 2; j++) {
        copy_to_user((char *) original_text_addr + (sc_entry_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (sc_entry_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (sc_entry_addr - text_section_offset) + j, nop_buffer, 1);
    }
    #endif

    #ifdef DISABLE_SC_EXIT
    for(j = 0; j < 2; j++) {
        copy_to_user((char *) original_text_addr + (sc_exit_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (sc_exit_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (sc_exit_addr - text_section_offset) + j, nop_buffer, 1);
    }
    #endif

    #ifdef HARDENING_VIEW
    for(j = 0; j < 2; j++) {
        copy_to_user((char *) original_text_addr + (remove_ss_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (remove_ss_addr - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (remove_ss_addr - text_section_offset) + j, nop_buffer, 1);
    }
    #endif

    #ifdef DISABLE_SS_LOGGING
    for(j = 0; j < 2; j++) {
        copy_to_user((char *) original_text_addr + (ss_entry_ud2 - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (ss_entry_ud2 - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (ss_entry_ud2 - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) original_text_addr + (ss_exit_ud2 - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ss_text_addr + (ss_exit_ud2 - text_section_offset) + j, nop_buffer, 1);
        copy_to_user((char *) ud2_text_addr + (ss_exit_ud2 - text_section_offset) + j, nop_buffer, 1);
    }
    #endif

    dt_create_kernel_pages(vcpu, ORIGINAL, original_text_addr);
    dt_create_kernel_pages(vcpu, HARDENED, ss_text_addr);
    dt_create_kernel_pages(vcpu, UD2, ud2_text_addr);
}

void initialize_proc_info(void) {
    int i;
    for(i = 0; i < MAX_PID; i++) {
        processes[i].is_set = false;
    }
}

struct proc_info * get_proc_info(int pid) {
    if(pid < MAX_PID || pid >= MAX_PID) {
        write_to_log_sprintf(fd_log, "pid is %d\n", pid);
        dt_printk("pid is %d\n*********************\n", pid);
        return &processes[0];
    }    
    if(processes[pid].is_set) return &processes[pid];
    return NULL;
}

struct proc_info * get_or_create_proc_info(int pid) {
    struct proc_info * pi;
    if(pid < 0 || pid >= MAX_PID) {
        write_to_log_sprintf(fd_log, "pid is %d\n", pid);
        dt_printk("pid is %d\n*********************\n", pid);
        return &processes[0];
    }
    pi = &processes[pid];
    if(pi->is_set) return pi;
    pi->is_set = true;
    pi->pid = pid;
    pi->curr_state = ORIGINAL;
    pi->curr_context = -1;
    pi->is_hardened = false;
    return pi;
}

bool dt_handle_switch_ctx(struct kvm_vcpu * vcpu, unsigned long instp) {
    struct proc_info * pi;
    #ifdef TIME_IT
    START_TIMER();
    #endif

    /* Check if the current UD2 exit is caused by a context switch on guest */
    if(instp == switch_context_ud2_addr) {
        /* Only called during the first context switch exit */
        if(first_time_ctx_handling) {
            first_time_ctx_handling = false;
            handle_first_time_ctx_handling(vcpu);
            initialize_proc_info();
        }
        #ifdef TIME_IT
        set_curr_proc_name(vcpu);
        #endif

        /* read curr_pid and tracking_enabled from guest */
        set_curr_proc_info(vcpu); 
       /* if tracking_enabled is true context switch to tracked process */
        /* if tracking_enabled is false context switch from tracked process */
        write_to_log_sprintf(fd_log, "switch_context_ud2_addr %d\n", tracking_enabled);
        if(tracking_enabled) {
            write_to_log_sprintf(fd_log, "context switch -> %s %d %d\n", curr_proc_name, curr_pid, tracking_enabled);
            pi = get_or_create_proc_info(curr_pid);

            /* The last time this process was executing, hardening was enabled */
            if(pi->is_hardened) {
                #ifndef PROFILING
                in_ctx = true;
                dt_change_scope(vcpu, HARDENED, -1);
                in_ctx = false;
                #endif
            } else {
                /* Restore old state if not profiling */
                #ifndef PROFILING
                in_ctx = true;
                #ifdef APPLICATION_SPECIALIZATION
                pi->curr_state = UD2;
                pi->curr_context = 0;
                #endif
                dt_change_scope(vcpu, pi->curr_state, pi->curr_context);
                in_ctx = false;
                #endif
            }
            write_to_log_sprintf(fd_log, "Context Switch %d : next_state %d\n", curr_pid, pi->curr_state);
            if(pi->curr_state != ORIGINAL) {
                /* If profiling, enable logging of guest kernel functions */
                #ifdef PROFILING
                start_logging(vcpu, ORIGINAL);
                #endif
            }
        } else {
            /* We are context switching from a tracked process to an untracked process on the guest */
            /* If not profiling, restore original kernel */
            #ifndef PROFILING
            in_ctx = true;
            dt_change_scope(vcpu, ORIGINAL, -1);
            in_ctx = false;
            #endif
            write_to_log_sprintf(fd_log, "Context Switch %d : next_state %d\n", curr_pid, ORIGINAL);
            
            /* If profiling, disable logging of guest kernel functions  */
            #ifdef PROFILING
            stop_logging(vcpu, ORIGINAL);
            #endif
        }
    } else {
        return false;
    }
    #ifdef TIME_IT
    END_TIMER(switch_ctx_time);
    #endif
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    num_switch_ctx_insts++;
    return true;
}

int set_and_get_syscall_id(struct kvm_vcpu * vcpu) {
    syscall_id = kvm_register_read(vcpu, VCPU_REGS_RDI);
    return syscall_id;
}

int get_syscall_id(void) {
    return syscall_id;
}

void get_curr_fn_name(struct kvm_vcpu * vcpu, char * curr_fn_name) {
    copy_from_user(curr_fn_name, (char *) dt_get_hva(vcpu, curr_fn_name_addr), 100);
}


void log_function(int sc_id, char * fn_name) {
    int buf_len = 0;
    char buffer[100];
    profile_sprintf(fd_log, "logging function : ");
    sprintf(buffer, "%d", sc_id);
    buf_len += strlen(buffer);
    buffer[buf_len] = ':';
    buf_len++;
    memcpy(buffer + buf_len, fn_name, strlen(fn_name));
    buf_len += strlen(fn_name);
    buffer[buf_len] = '\n';
    buf_len++;
    profile(fd_log, buffer, buf_len);    
}

#ifdef LARGE_FILE
void init_syscall_funcs(void) {
    int i;
    if(syscall_funcs_initialized) return;
    for(i = 0; i < num_sys_calls; i++) {
        syscall_num_funcs[i] = 0;
    }
    syscall_funcs_initialized = true;
}

void add_logged(int id, char * fname) {
    char * str;
    if(id < 0) {
        str = kmalloc(sizeof(char) * strlen(fname) + 1, GFP_KERNEL);
        memcpy(str, fname, strlen(fname) + 1);
        global_funcs[global_num_funcs] = str;
        global_num_funcs++;
        return;
    }
    str = kmalloc(sizeof(char) * strlen(fname) + 1, GFP_KERNEL);
    memcpy(str, fname, strlen(fname) + 1);
    syscall_funcs[id][syscall_num_funcs[id]] = str;
    syscall_num_funcs[id]++;
}

bool check_logged(int sc_id, char * fname) {
    int i;
    if(num_irqs || in_exc) {
        for(i = 0; i < global_num_funcs; i++) {
            if(!strcmp(global_funcs[i], fname)) return true;
        }
        // profile_sprintf(fd_log, "adding function to global_funcs %s\n", fname);
        return false;
    }
    if(sc_id < 0 || sc_id >= num_sys_calls) {
        write_to_log_sprintf(fd_log, "Unexpected Scenario sc_id is %d\n", sc_id);
        return false;
    }
    for(i = 0; i < syscall_num_funcs[sc_id]; i++) {
        if(!strcmp(syscall_funcs[sc_id][i], fname)) return true;
    }
    return false;
}
#endif

bool of_interest = false;
void print_pfault_info(struct kvm_vcpu * vcpu) {
    #ifndef DISABLE_LOGGING
    unsigned long  code = kvm_register_read(vcpu, VCPU_REGS_R15);
    unsigned long  address = kvm_register_read(vcpu, VCPU_REGS_R12);
    write_to_log_sprintf(fd_log, "code is %ld, address is %lx\n", code, address);
    write_to_log_sprintf(fd_log, "protection bit = %ld\n", code & X86_PF_PROT);
    #endif
    if(of_interest) {
        print_lbr();
    }
}

void entered_syscall(void) {
    in_exc = false;
    in_syscall = true;
}

void exited_syscall(void) {
    in_exc = true;
    in_syscall = false;
}

bool dt_handle_irq_func(struct kvm_vcpu * vcpu, unsigned long instp) {
    if(instp == log_irq_enter_addr) {
        num_irqs++;
    } else if(instp == log_irq_exit_addr) {
        if(num_irqs == 0) {
        } else {
            num_irqs--;
        }
    } else {
        return false;
    }
    irq_exits++;
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    return true;
}

void dt_handle_logging(struct kvm_vcpu * vcpu) {
    char curr_fn_name[100];
    int id, sc_id = get_syscall_id();
    if(num_irqs) id = -1;
    else if(in_exc) id = -2;
    else id = sc_id;
    get_curr_fn_name(vcpu, curr_fn_name);
    write_to_log_sprintf(fd_log, "func is %s id is %d status is %d\n", curr_fn_name, id, print_status(vcpu, log_irq_enter_addr, ORIGINAL));
    // if(!strcmp(curr_fn_name, "irq_enter")) {
    //     nop_to_ud2();
    // }
    #ifdef LARGE_FILE
    init_syscall_funcs();
    if(check_logged(sc_id, curr_fn_name)) return;
    add_logged(id, curr_fn_name);
    #endif
    log_function(id, curr_fn_name);
}

void print_do_fork(struct kvm_vcpu * vcpu) {
    bool my_variable_to_update = 0;
    copy_from_user(&my_variable_to_update, (void *) dt_get_hva(vcpu, 0xffffffff86af97b0 + 5000), 1);
    dt_printk("value of interest is %d %s\n", my_variable_to_update, curr_proc_name);
    write_to_log_sprintf(fd_log, "value of interest is %d %s\n", my_variable_to_update, curr_proc_name);
}

// void handle_do_fork(struct kvm_vcpu * vcpu) {
//  set_curr_proc_name(vcpu);
//     copy_to_user((void *) dt_get_hva(vcpu, do_fork_addr), nop_buffer, 1);
//     copy_to_user((void *) dt_get_hva(vcpu, do_fork_addr + 1), nop_buffer, 1);
//  if(!strcmp(curr_proc_name, "exploit")) {
//      print_do_fork(vcpu);
//      kvm_register_write(vcpu, VCPU_REGS_RIP, do_fork_addr + 2);
//  } else  {
//         // kvm_register_write(vcpu, VCPU_REGS_RIP, 0xffffffff8126f62b);
//  }
// }

// void handle_st2(struct kvm_vcpu * vcpu) {
//     copy_to_user((void *) dt_get_hva(vcpu, set_tracking_2_addr), nop_buffer, 1);
//     copy_to_user((void *) dt_get_hva(vcpu, set_tracking_2_addr + 1), nop_buffer, 1);
// }


// void handle_jsk(struct kvm_vcpu * vcpu, unsigned long instp) {
//  set_curr_proc_name(vcpu);
//  if(!strcmp(curr_proc_name, "exploit")) {
//      kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
//  } /*else  {
//      kvm_register_write(vcpu, VCPU_REGS_RIP, 0xffffffff8125305b);
//  }
// }

bool dt_handle_sc(struct kvm_vcpu * vcpu, unsigned long instp) {
    int sc_id;
    int shadow_stack_index;
    struct proc_info * pi;
    #ifdef TIME_IT
    START_TIMER();
    #endif

    /* Entered a system call */
    if(instp == sc_entry_addr) {

        /* Obtain system call id by reading the %rdi register on guest */
        sc_id = set_and_get_syscall_id(vcpu);

        if(sc_id == 59) goto UPDATE_INST;
        pi = get_or_create_proc_info(curr_pid);

        #ifndef HARDENING_VIEW
        pi->curr_state = UD2;
        pi->curr_context = sc_id;
        pi->is_hardened = false;
        #else
        pi->curr_state = HARDENED;
        pi->curr_context = -1;
        pi->is_hardened = true;
        #endif

        #ifdef PROFILING
            entered_syscall();
        #endif

        write_to_log_sprintf(fd_log, "entering system call %d for proc %d\n", sc_id, curr_pid);
        if(sc_id == 247) {
            print_do_fork(vcpu);
        }

        #ifndef PROFILING
        in_sc = true;
        /* Specialize the kernel view with respect to the system-call application pair */
        dt_change_scope(vcpu, pi->curr_state, pi->curr_context);
        in_sc = false;
        #endif

        #ifdef PROFILING
        start_logging(vcpu, ORIGINAL);
        #endif
        num_sc_entry++;
    } else if(instp == sc_exit_addr) {
        sc_id = set_and_get_syscall_id(vcpu);
        if(sc_id == 59) goto UPDATE_INST;
        pi = get_or_create_proc_info(curr_pid);
        pi->curr_state = ORIGINAL;
        pi->curr_context = -1;

        #ifdef PROFILING
            exited_syscall();
        #endif
        
        write_to_log_sprintf(fd_log, "exiting system call %d\n", sc_id);
        if(sc_id == 247) {
            print_do_fork(vcpu);
        }
        #ifndef PROFILING
        dt_change_scope(vcpu, pi->curr_state, pi->curr_context);
        #endif
        copy_from_user((char *) &shadow_stack_index, (char *) dt_get_hva(vcpu, shadow_stack_index_addr), 4);
        if(shadow_stack_index != 0) {
            write_to_log_sprintf(fd_log, "ERROR : shadow_stack_index is not equal to zero when exiting\n");
        }
        if(pi->is_hardened) {
            write_to_log_sprintf(fd_log, "ERROR : Kernel is  hardened upon system call exit\n");
        }
        // #ifdef PROFILING
        // stop_logging(vcpu, ORIGINAL);
        // #endif
        num_sc_exit++;
    }
    else if(instp == log_fn_addr) {
        num_logging_exits++;
        write_to_log_sprintf(fd_log, "logging exit\n");
        dt_handle_logging(vcpu);
    } else {
        return false;
    }
    UPDATE_INST:
    #ifdef TIME_IT
    END_TIMER(sc_time);
    #endif
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    return true;
}

unsigned long get_actual_stack_addr(struct kvm_vcpu * vcpu) {
    unsigned long actual_stack_addr, actual_stack_addr_ptr;
    actual_stack_addr_ptr = kvm_register_read(vcpu, VCPU_REGS_RDI);
    copy_from_user((char *) &actual_stack_addr, (char *) dt_get_hva(vcpu, actual_stack_addr_ptr), 8);
    return actual_stack_addr;
}

bool dt_handle_ss_funcs_2(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    int shadow_stack_index;
    unsigned long called_from, actual_stack_addr, shadow_stack_value, addr_to_write;
    struct func_node * fn;
    struct proc_info * pi = get_or_create_proc_info(curr_pid);

    if(instp == ss_entry_ud2) {
        copy_from_user((char *) &shadow_stack_index, (char *) dt_get_hva(vcpu, shadow_stack_index_addr), 4);
        copy_from_user((char *) &called_from, (char *) dt_get_hva(vcpu, stack_addr), 8);
        fn = dt_get_fnode(called_from, fn_array, 0, fn_array_size);
        actual_stack_addr = get_actual_stack_addr(vcpu);
        if(fn) {
            write_to_log_sprintf(fd_log, "handling ss_entry %d %s %lx\n", shadow_stack_index, fn->name, actual_stack_addr);
        } else {
            write_to_log_sprintf(fd_log, "handling ss_entry ERROR : no fnode %d %lx %lx\n", shadow_stack_index, called_from, actual_stack_addr);
        }
        if(!pi->is_hardened) {
            write_to_log_sprintf(fd_log, "ERROR : ss_entry called even though kernel is not hardened\n");
        }
        // if(!strcmp(fn->name, "prepare_kernel_cred")) {
        //     write_to_log_sprintf(fd_log, "prepare_kernel_cred %lx\n", kvm_register_read(vcpu, VCPU_REGS_R12));
        //     kvm_register_write(vcpu, VCPU_REGS_R12, 0);
        // }

    } else if(instp == ss_exit_ud2) {
        copy_from_user((char *) &shadow_stack_index, (char *) dt_get_hva(vcpu, shadow_stack_index_addr), 4);
        copy_from_user((char *) &called_from, (char *) dt_get_hva(vcpu, stack_addr), 8);
        fn = dt_get_fnode(called_from, fn_array, 0, fn_array_size);
        actual_stack_addr = get_actual_stack_addr(vcpu);
        copy_from_user((char *) &shadow_stack_value, (char *) dt_get_hva(vcpu, shadow_stack_addr), 8);
        copy_from_user((char *) &addr_to_write, (char *) dt_get_hva(vcpu, shadow_stack_value + (shadow_stack_index - 1) * 8), 8);
        if(fn) {
            write_to_log_sprintf(fd_log, "handling ss_exit %d %s %lx %lx\n", shadow_stack_index, fn->name, actual_stack_addr, addr_to_write);
        } else {
            write_to_log_sprintf(fd_log, "ERROR : no fnode %d %lx %lx %lx\n", shadow_stack_index, called_from, actual_stack_addr, addr_to_write);
        }
        // if(!strcmp(fn->name, "get_root_fork_ss_1")) {
        //     write_to_log_sprintf(fd_log, "%lx\n", kvm_register_read(vcpu, VCPU_REGS_RDI));
        //     kvm_register_write(vcpu, VCPU_REGS_RDI, 0);
        //     of_interest = true;
        // }
        if(actual_stack_addr != addr_to_write) {
            write_to_log_sprintf(fd_log, "ERROR : actual and shadow are not equal\n");
        }
        if(!pi->is_hardened) {
            write_to_log_sprintf(fd_log, "ERROR : ss_exit called even though kernel is not hardened\n");
        }
    } else {
        return false;
    }
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    return true;
}

bool dt_handle_ss_funcs(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    int shadow_stack_index;
    struct func_node * fn;
    unsigned long called_from, actual_stack_addr, shadow_stack_value, addr_to_write;
    struct proc_info * pi = get_or_create_proc_info(curr_pid);

    if(instp == pi->ret_addr && pi->is_hardened) {
        pi->is_hardened = false;
        dt_change_scope(vcpu, pi->curr_state, pi->curr_context);
        write_to_log_sprintf(fd_log, "Removed Shadow stack %d : next_state %d %lx\n", curr_pid, pi->curr_state, instp);
        shadow_stack_index = 0;
        // copy_to_user((char *) dt_get_hva(vcpu, shadow_stack_index_addr), (char *) &shadow_stack_index, 4);
        if(instp >= text_section_offset && instp < text_section_offset + text_section_size) {
            ts_read(instp, ORIGINAL, pi->ret_buf, 2);
            ts_write(instp, HARDENED, pi->ret_buf, 2);
        } else {
            copy_to_user((void *) dt_get_hva(vcpu, pi->ret_addr), pi->ret_buf, 2);
        }
        num_ss_exits++;
        return true;
    } else if(instp == remove_ss_addr) {
        actual_stack_addr = get_actual_stack_addr(vcpu); 
        if(pi->ret_addr == actual_stack_addr) {
            copy_from_user((char *) &called_from, (char *) dt_get_hva(vcpu, stack_addr), 8);
            fn = dt_get_fnode(called_from, fn_array, 0, fn_array_size);
            write_to_log_sprintf(fd_log, "Setting up for shadow stack removal %s %lx\n", fn->name, pi->ret_addr);
            if(pi->ret_addr >= text_section_offset && pi->ret_addr < text_section_offset + text_section_size) {
                ts_write(pi->ret_addr, HARDENED, ud2_buffer, 2);
            } else {
                copy_from_user(pi->ret_buf, (void *) dt_get_hva(vcpu, pi->ret_addr), 2);
                copy_to_user((void *) dt_get_hva(vcpu, pi->ret_addr), ud2_buffer, 2);
            }
        }
    } else if(instp == ss_fault_addr) {
        copy_from_user((char *) &shadow_stack_index, (char *) dt_get_hva(vcpu, shadow_stack_index_addr), 4);
        copy_from_user((char *) &called_from, (char *) dt_get_hva(vcpu, stack_addr), 8);
        fn = dt_get_fnode(called_from, fn_array, 0, fn_array_size);
        actual_stack_addr = get_actual_stack_addr(vcpu);        
        copy_from_user((char *) &shadow_stack_value, (char *) dt_get_hva(vcpu, shadow_stack_addr), 8);
        copy_from_user((char *) &addr_to_write, (char *) dt_get_hva(vcpu, shadow_stack_value + (shadow_stack_index - 1) * 8), 8);
        if(fn) {
            write_to_log_sprintf(fd_log, "Not Equal : %d %s %lx %lx\n", shadow_stack_index, fn->name, actual_stack_addr, addr_to_write);
        } else {
            write_to_log_sprintf(fd_log, "Not equal : %d %lx %lx %lx\n", shadow_stack_index, called_from, actual_stack_addr, addr_to_write);
        }
        return true;
    } else {
        return false;
    }
    num_ss_exits++;
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    return true;
}

void dt_handle_illegal_transfer(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    unsigned long ret_addr;
    #ifndef DISABLE_LOGGING
    struct func_node * fn = dt_get_fnode(instp, fn_array, 0, fn_array_size);
    #endif
    ret_addr = dt_get_stack_addr(vcpu, stack_addr);
    write_to_log_sprintf(fd_log, "Alert : control flow anomaly - Hijacking attempt? at %lx\n", fn->address);
    write_to_log_sprintf(fd_log, "Redirecting to %lx\n", ret_addr);
    kvm_register_write(vcpu, VCPU_REGS_RIP, ret_addr);
    kvm_register_write(vcpu, VCPU_REGS_RSP, stack_addr + 8);
}

#define threshold 100
int num_r = 0;
int r_nums[10000];
char * r_names[10000];

void print_should_bring_to_memory(void) {
    int i;
    for(i = 0; i < num_r; i++) {
        if(r_nums[i] > threshold) {
            dt_printk("should bring to memory: %s %d\n", r_names[i], r_nums[i]);
        }
    }
}

void set_recovered(char * name) {
    int i;
    for(i = 0; i < num_r; i++) {
        if(!strcmp(r_names[i], name)) {
            r_nums[i] += 1;
            return;
        }
    }
    r_names[num_r] = name;
    r_nums[num_r] = 1;
    num_r += 1;
}

// uint64_t undefined_time = 0;
bool dt_handle_unexplored(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    char buffer[10000];
    struct proc_info * pi;
    unsigned long ret_addr; 
    // unsigned long transfer_from;
    // struct func_node * caller;
    struct func_node * fn = dt_get_fnode(instp, fn_array, 0, fn_array_size);
    if(!fn) {
        write_to_log_sprintf(fd_log, "not found %lx\n", instp);
        return false;
    }

    ts_read(instp, ORIGINAL, buffer, 2);
    if(buffer[0] == ud2_buffer[0] && buffer[1] == ud2_buffer[1]) {
        write_to_log_sprintf(fd_log, "will not recover function at %lx\n", instp);
        return false;
    }

    if(instp != fn->address) {
        ts_read(fn->address, ORIGINAL, buffer, fn->code_size);
        ts_write(fn->address, UD2, buffer, fn->code_size);
        write_to_log_sprintf(fd_log, "Bringing function into memory %lx %s\n", instp, fn->name);
        return true;
    }
    ret_addr = dt_get_stack_addr(vcpu, stack_addr);
    // transfer_from = ret_addr;
    // transfer_from = get_transfer_from(instp);   
    // START_TIMER();
    // if(!transfer_from) transfer_from = ret_addr;
    // if(!check_transfer(vcpu, transfer_from, instp)) {
    //     caller = dt_get_fnode(transfer_from, fn_array, 0, fn_array_size);
    //     write_to_log_sprintf(fd_log, "jump out of the call graph %s -> %s\n", caller->name, fn->name);
    // }
    pi = get_or_create_proc_info(curr_pid);
    pi->is_hardened = true;
    pi->ret_addr = ret_addr;
    #ifndef HARDENING_ORIGINAL
    dt_change_scope(vcpu, HARDENED, -1);
    #else
    dt_change_scope(vcpu, ORIGINAL, -1);
    #endif
    set_recovered(fn->name);
    write_to_log_sprintf(fd_log, "Recovering function - %s start of function - %lx, instp - %lx\n", fn->name, fn->address, instp);
    num_recovered++;
    // END_TIMER(undefined_time);
    return true;
}

bool dt_handle_fcntl(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    if(instp == 0xffffffff81595a0b) {
        unsigned long ret_addr = dt_get_stack_addr(vcpu, stack_addr);
        dt_printk("address on the stack is %lx\n", ret_addr);
        dt_printk("***************************************");
        kvm_register_write(vcpu, VCPU_REGS_RSP, stack_addr + 8);
        kvm_register_write(vcpu, VCPU_REGS_RBX, ret_addr);
    } else if(instp == 0xffffffff8155669c) {
    } else {
        return false;
    }
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    return true;  
}



// void print_ht_at_id(struct kvm_vcpu * vcpu, int id) {
//     uint64_t hash_table_addr;
//     uint8_t bytes[4096];
//     unsigned i, j;
//     int num_used = 0;
//     for(i = 0; i < num_ts_frames; i++) {
//         copy_from_user(&hash_table_addr, (char *) dt_get_hva(vcpu, 0xffffffff86c59010 + id * num_ts_frames * 8 + i * 8), 8);
//         if(!hash_table_addr) continue;
//         copy_from_user(bytes, (char *) dt_get_hva(vcpu, hash_table_addr), 4096);
//         for(j = 0; j < 4096; j++) {
//             if(!bytes[j]) continue;
//             dt_printk("address is %lx\n", text_section_offset + i * 4096 + j);
//             num_used++;
//         }
//     }
//     dt_printk("num_used %d\n", num_used);
//     dt_printk("********************************\n");
// }

// bool check_ci(struct kvm_vcpu * vcpu) {
//     uint64_t curr_fn_ptr, frame, offset;
//     int id;
//     uint64_t hash_table_addr;
//     uint8_t bytes[4096];

//     id = kvm_register_read(vcpu, VCPU_REGS_RBX);
//     curr_fn_ptr = kvm_register_read(vcpu, VCPU_REGS_R14);
//     write_to_log_sprintf(fd_log, "id is %d curr_fn_ptr %llx\n", id, curr_fn_ptr);
//     frame = FRAME(curr_fn_ptr);
//     offset = OFFSET(curr_fn_ptr);

//     if(id < 0 || id >= 5733) {
//         write_to_log_sprintf(fd_log, "ERROR : id\n");
//         return false;
//     } 
//     if(frame > num_ts_frames) {
//         write_to_log_sprintf(fd_log, "ERROR : frame\n");
//         return false;        
//     } 
//     if(offset > 4096) {
//         write_to_log_sprintf(fd_log, "ERROR : offset\n");
//         return false;           
//     } 
//     copy_from_user(&hash_table_addr, (char *) dt_get_hva(vcpu, 0xffffffff81e1b000 + id * num_ts_frames * 8 + frame * 8), 8);
//     if(!hash_table_addr) {
//         write_to_log_sprintf(fd_log, "NOT MATCHED : frame\n");
//         return false;
//     }
//     copy_from_user(bytes, (char *) dt_get_hva(vcpu, hash_table_addr), 4096);
//     if(!bytes[offset]) {
//         write_to_log_sprintf(fd_log, "NOT MATCHED : offset\n");
//         return false;
//     }
//     return true;
// } 

bool dt_handle_cfi(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    unsigned long curr_fn_ptr, ret_addr;
    struct func_node * fn1, * fn2;
    if(instp == initialize_addr) {
        // nop_to_ud2(vcpu, check_ci_addr, ORIGINAL);
    } else if(instp == check_ci_addr) {
        ret_addr = dt_get_stack_addr(vcpu, stack_addr);
        fn1 = dt_get_fnode(ret_addr, fn_array, 0, fn_array_size);
        copy_from_user((void *) &curr_fn_ptr, (void *) dt_get_hva(vcpu, cfi_invalid_addr), 8);
        fn2 = dt_get_fnode(curr_fn_ptr, fn_array, 0, fn_array_size);
        if(fn1 && fn2) {
            write_to_log_sprintf(fd_log, "invalid cfi transfer from %s to %s\n", fn1->name, fn2->name);
        } else {
            write_to_log_sprintf(fd_log, "invalid cfi transfer to unknown address %lx %lx\n", ret_addr, curr_fn_ptr);
        }
    } else {
        return false;
    }
    kvm_register_write(vcpu, VCPU_REGS_RIP, instp + 2);
    return true;
}
/*
bool dt_can_handle(struct kvm_vcpu *vcpu, unsigned long instp) {
    char buffer[100];
    struct proc_info * pi;

    pi = get_proc_info(curr_pid);
    if(pi && !first_time_ctx_handling) {
        ts_read(instp, ORIGINAL, buffer, 100);
        if(pi->ret_addr == instp && pi->is_hardened)
            return true;
        if(buffer[0] == ud2_buffer[0] && buffer[1] == ud2_buffer[1])
            return true;
        if(instp < text_section_offset || instp >= text_section_offset + text_section_size) {
            return false;
        }
        if(cont_info[pi->curr_context].frames[FRAME(instp)]) {
            ts_read(instp, UD2, buffer, 2);
            if(buffer[0] == ud2_buffer[0] && buffer[1] == ud2_buffer[1])
                return true;
        }
    }
    return instp == switch_context_ud2_addr || instp == log_fn_addr;
}
*/
// int set_page_rw(unsigned long addr)
// {
//     unsigned int level;
//     pte_t *pte = lookup_address(addr, &level);
//     if (pte->pte &~ _PAGE_RW) pte->pte |= _PAGE_RW;
//     return 0;
// }


/* Entry point of the code */

bool dt_handle_exception(struct kvm_vcpu * vcpu, unsigned long instp, unsigned long stack_addr) {
    #ifdef TIME_IT
    START_TIMER();
    #endif

    write_to_log_sprintf(fd_log, "instp is %lx\n", instp);
    /* Called when there is a context switch on the guest for the first time. Initialize guest tracked procs */
    if(instp == initialize_addr) {
        dt_printk("handling initialize_addr\n");
        dt_handle_cfi(vcpu, instp, stack_addr);
        initialize_kernel_paths();
        print_guest_tracked_procs(vcpu);
        set_guest_tracked_procs(vcpu);
        print_guest_tracked_procs(vcpu);
        return true;
    }

    /* Early hardware exceptions - We do not want to initialize before first context switch exit */
    if(instp == 0xfd099 || instp == 0xfd0ae || (first_time_ctx_handling && instp != switch_context_ud2_addr)) {
        return false;
    }

    // if(!dt_can_handle(vcpu, instp)) {
    //     dt_printk("returning false %lx\n", instp);
    //     dt_printk("******************************************\n");
    //     return false;
    // }
    
    /* Initialize data structures. Read systsem call dependency information, control flow graph.
    Initialize data structures in mmu.c */
    if(!fn_array_size) {
        dt_initialize(vcpu);
    }
    HAS_HANDLED = 
    /* Context switch exit */
    dt_handle_switch_ctx(vcpu, instp)
    /* System call exit */ 
    || dt_handle_sc(vcpu, instp)
    /* Shadow stack violation or Shadow stack removal */ 
    || dt_handle_ss_funcs(vcpu, instp, stack_addr)
    /* Debugging through shadow stack - what functions on guest called ss_entry and ss_exit? */
    || dt_handle_ss_funcs_2(vcpu, instp, stack_addr)
    /* CFI violation */
    || dt_handle_cfi(vcpu, instp, stack_addr)
    // || dt_handle_fcntl(vcpu, instp, stack_addr)
    #ifdef PROFILING
    /* When profiling, check if an interrupt is encountered on the guest */
    || dt_handle_irq_func(vcpu, instp)
    #endif
    /* Unexplored code path */
    || dt_handle_unexplored(vcpu, instp, stack_addr);
    write_to_log_sprintf(fd_log, "HAS_HANDLED is %d\n", HAS_HANDLED);
    if(HAS_HANDLED) {
        num_handled++;
        if(num_handled % 10000 == 0) {
            print_should_bring_to_memory();        
            print_numbers();
        }
        #ifdef TIME_IT
        END_TIMER(handling_time);
        #endif      
    }
    return HAS_HANDLED;
}
