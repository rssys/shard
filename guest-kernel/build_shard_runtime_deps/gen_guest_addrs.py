import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile

def get_addr_of(fname, sname):
	p1 = subprocess.Popen(["llvm-nm", fname], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", sname], stdin=p1.stdout, stdout=subprocess.PIPE)
	addr = p2.communicate()[0]
	return addr  


def get_function_ud2(symbol, dump):
	p = subprocess.Popen(["grep", "-A", "1000", symbol, dump], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	output = p.communicate()
	lines = output[0].split("\n")
	for line in lines:
		if line.find("0f 0b") >= 0 and line.find("ud2") >= 0:
			return line[:16]
		if line == "":
			break
	return None

def print_inst_addr(str, sym, dump):
	addr = get_function_ud2(sym, dump)
	if addr != None:
		print str, "0x"+ addr
	else:
		print str, "0xffffffffffffffff"

def print_var_addr(str, sym, vmfile):
	addr = get_addr_of(vmfile, sym)
	if addr != None and addr != "":
		print str, "0x" + addr[:16]
	else:
		print str, "0xffffffffffffffff"

def two_nops(line, next_line):
	if line.find("nop") >= 0 and line.find("90") >= 0 and next_line.find("nop") >= 0 and next_line.find("90") >= 0:
		return True
	return False
vmfile = sys.argv[1]
dump = sys.argv[2]

print_inst_addr("#define switch_context_ud2_addr", "ud2_call>:", dump);
print_inst_addr("#define sc_entry_addr", 'sc_entry>:', dump)
print_inst_addr("#define sc_exit_addr", 'sc_exit>:', dump)
print_inst_addr("#define ss_entry_ud2", 'ss_entry>:', dump)
print_inst_addr("#define ss_exit_ud2", 'ss_exit>:', dump)
print_inst_addr("#define save_old_ss_ud2", 'save_old_ss>:', dump)
print_inst_addr("#define add_new_ss_ud2", 'add_new_ss>:', dump)
print_inst_addr("#define check_ci_addr", 'check_ci>:', dump)
print_inst_addr("#define initialize_addr", '<initialize>:', dump)

print_var_addr("#define curr_proc_name_addr", 'curr_prname', vmfile)
print_var_addr("#define curr_pid_addr", "curr_pid", vmfile)
print_var_addr("#define curr_sc_id_addr", "curr_sc_id", vmfile)
print_var_addr("#define track_syscalls_addr", "track_syscalls", vmfile)
print_var_addr("#define curr_fn_name_addr", "curr_fn_name", vmfile) 

print_var_addr("#define shadow_stack_addr", "shadow_stack", vmfile)
print_var_addr("#define shadow_stack_index_addr", "shadow_stack_index", vmfile)
print_var_addr("#define shadow_stack_pool_addr", "shadow_stack_pool", vmfile)
print_var_addr("#define ss_curr_pid", "ss_curr_pid", vmfile)
print_var_addr("#define num_procs_addr", "num_procs", vmfile)
print_var_addr("#define curr_func_ptr_table_idx_addr", "curr_func_ptr_table_idx", vmfile)
print_var_addr("#define curr_func_ptr_addr", "curr_func_ptr", vmfile)
print_var_addr("#define cfi_invalid_addr", "invalid_addr", vmfile)
print_var_addr("#define num_tracked_procs_addr", " num_tracked_procs", vmfile)
print_var_addr("#define tracked_procs_addr", " tracked_procs", vmfile)
print_var_addr("#define syscall_entry_time_addr", " syscall_entry_time", vmfile)
print_var_addr("#define syscall_exit_time_addr", " syscall_exit_time", vmfile)
print_var_addr("#define ctx_switch_time_addr", " ctx_switch_time", vmfile)
print_var_addr("#define are_we_tracking_addr", " are_we_tracking", vmfile)

p = subprocess.Popen(["grep", "-A", "1000", "log_fn>:", dump], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
output = p.communicate()
lines = output[0].split("\n")
addr = None
for idx, line in enumerate(lines):
	if idx < len(lines) - 1 and two_nops(line, lines[idx + 1]):
		addr = line[:16]
		break
		
if addr != None:
	print "#define log_fn_addr", "0x"+ addr 
else:
	print "#define log_fn_addr", "0xffffffffffffffff"


p = subprocess.Popen(["grep", "-A", "1000", "log_irq_enter>:", dump], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
output = p.communicate()
lines = output[0].split("\n")
addr = None
for idx, line in enumerate(lines):
	if idx < len(lines) - 1 and two_nops(line, lines[idx + 1]):
		addr = line[:16]
		break
		
if addr != None:
	print "#define log_irq_enter_addr", "0x"+ addr 
else:
	print "#define log_irq_enter_addr", "0xffffffffffffffff"


p = subprocess.Popen(["grep", "-A", "1000", "log_irq_exit>:", dump], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
output = p.communicate()
lines = output[0].split("\n")
addr = None
for idx, line in enumerate(lines):
	if idx < len(lines) - 1 and two_nops(line, lines[idx + 1]):
		addr = line[:16]
		break
		
if addr != None:
	print "#define log_irq_exit_addr", "0x"+ addr 
else:
	print "#define log_irq_exit_addr", "0xffffffffffffffff"


p = subprocess.Popen(["grep", "-A", "1000", "ss_exit>:", dump], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
output = p.communicate()
lines = output[0].split("\n")
if not len(lines):
	symbol = "ss_fault_addr"
	print "#define", symbol, "0xffffffffffffffff"
	symbol = "remove_ss_addr"
	print "#define", symbol, "0xffffffffffffffff" 
else:
	count = 0
	for idx, line in enumerate(lines):
		if line == "":
			break
		if line.find("ud2") >= 0:
			count += 1
			addr = line[:16]
			if count == 1:
				continue
			elif count == 2:
				symbol = "ss_fault_addr"
			elif count == 3:
				symbol = "remove_ss_addr"
			else:
				print "found more than three ud2 in ss_exit"
				break
			print "#define", symbol, "0x"+ addr 
