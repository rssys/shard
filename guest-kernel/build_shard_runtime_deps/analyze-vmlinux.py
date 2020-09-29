import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile

file_offset = 0x200000 # 0x809000
text_size = 0x800000 # 12288
addr_lo = 0xffffffff81000000 # 0xffffffff81609000
addr_hi = addr_lo + text_size # 0xffffffff8160c000

to_skip_lo = -1
to_skip_hi = -1

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]


def get_addr_of(fname, sname):
	p1 = subprocess.Popen(["llvm-nm", fname], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", sname], stdin=p1.stdout, stdout=subprocess.PIPE)
	return p2.communicate()[0]

def add_curr_pr_address(fname):
	te_al = get_addr_of(fname, "tracking_enabled")
	cpn_al = get_addr_of(fname, "curr_prname")
	with open("/home/muhammad/testing/debloating/dt_ita_addr", 'w') as fd:
		print hex(int(te_al.split()[0], 16))
		print hex(int(cpn_al.split()[0], 16))
		fd.write(struct.pack("Q", int(te_al.split()[0], 16)))
		fd.write(struct.pack("Q", int(cpn_al.split()[0], 16)))

def get_func_names(fname, threshold):
	content = read_file(fname)
	stripped = map(lambda x : x.split(), content)
	func_names_is = map(lambda x : x[0], (filter(lambda x : (int(x[1]) <= 1000 and int(x[1]) > threshold), stripped)))
	func_names_oos = map(lambda x : x[0], (filter(lambda x : int(x[1]) <= threshold, stripped)))
	return func_names_is, func_names_oos

def create_copy(vmfile, suff):
	cpy = vmfile + suff
	copyfile(vmfile, cpy)
	return cpy


total_check_ci = to_skip_lo
def check_protection_inst(line):
	global  total_check_ci
	if line.find("callq") > 0 and (line.find("ss_entry") > 0 or line.find("ss_exit") > 0):
		return True
	if line.find("callq") > 0 and line.find("check_ci") > 0:
		if total_check_ci >= to_skip_lo and total_check_ci < to_skip_hi:
			total_check_ci += 1
			return False
		total_check_ci += 1
		return True
	return False

def get_symbol_info(vmfile):
	p = subprocess.Popen(["objdump", "-dF", vmfile.strip()], stdout=subprocess.PIPE)
	output = p.communicate()[0]
	output = output.split('\n')

	protection_insts = []
	pattern = re.compile("([a-z0-9]+) <([._a-zA-Z0-9]+)> \(File Offset: ([a-z0-9]+)\):")
	curr_func_file_offset = -1
	curr_func_bin_offset = -1

	for idx, line in enumerate(output):
		if line == "":
			continue
		toks = line.split("\t")
		if len(toks) >= 2:
			code_size = len(toks[1].split())
			if check_protection_inst(line):
				if curr_func_file_offset == -1 or curr_func_bin_offset == -1:
					print "unexpected scenario curr_func_file_offset, curr_func_bin_offset -1"
					sys.exit()
				toks = line.split('\t')
				inst_offset_bin = int(toks[0][:-1], 16)
				inst_offset_file = inst_offset_bin - curr_func_bin_offset + curr_func_file_offset
				protection_insts.append((inst_offset_file, inst_offset_bin))

			if line.find("jmpq") > 0 and (line.find("ss_entry") > 0 or line.find("ss_exit") > 0):
				print line
		matched = pattern.match(line)
		if not matched:
			continue
		func_name = matched.group(2)
		if(func_name == "ud2_call"):
			global switch_ctx_offset
			switch_ctx_offset = int(matched.group(3), 16)
			continue
		address = int(matched.group(1), 16)
		curr_func_bin_offset = int(matched.group(1), 16)
		curr_func_file_offset = int(matched.group(3), 16)
	return protection_insts



vmfile = sys.argv[1]

protection_insts = get_symbol_info(vmfile)

# with open("/home/muhammad/testing/debloating/ss_list", 'w') as fd:
# 	for _, offset in protection_insts:
# 		# print hex(offset)
# 		address = struct.pack("Q", offset)
# 		fd.write(address)
print "number of ss insts is ", len(protection_insts)
iis = create_copy(vmfile, "_is")
oos = create_copy(vmfile, "_oos")

with open(vmfile, "r+b") as fd:
	for  offset, _ in protection_insts:
		fd.seek(offset)
		fd.write(chr(int("0F", 16)))
		fd.write(chr(int("1F", 16)))
		fd.write(chr(int("44", 16)))
		fd.write(chr(int("00", 16)))
		fd.write(chr(int("00", 16)))

# with open(iis, "r+b") as fd:
# 	for (func_name, address, offset, is_is, inst_sizes) in func_info:
# 		offset = int(offset, 16)
# 		fd.seek(offset)
# 		code = fd.read(sum(inst_sizes))
# 		fd.seek(offset)
# 		code_idx = 0
# 		if not is_is:
# 			for isize in inst_sizes:
# 				if isize == 1: 
# 					fd.write(code[code_idx])
# 					code_idx += 1
# 					continue
# 				for x in xrange(isize/2):
# 					fd.write(chr(int("0F", 16)))
# 					fd.write(chr(int("0B", 16)))
# 					code_idx += 2
# 				if isize % 2:
# 					fd.write(code[code_idx])
# 					code_idx += 1
			# if inst_sizes[0] == 1:
			# 	continue
			# fd.write(chr(int("0F", 16)))
			# fd.write(chr(int("0B", 16)))
		# to_store.append((func_name, address, code, is_is, inst_sizes))

# with open(oos, "r+b") as fd:
# 	for (func_name, address, offset, is_is, inst_sizes) in func_info:
# 		offset = int(offset, 16)
# 		fd.seek(offset)
# 		code = fd.read(sum(inst_sizes))
# 		fd.seek(offset)
# 		code_idx = 0
# 		if is_is:
# 			for isize in inst_sizes:
# 				if isize == 1: 
# 					fd.write(code[code_idx])
# 					code_idx += 1
# 					continue
# 				for x in xrange(isize/2):
# 					fd.write(chr(int("0F", 16)))
# 					fd.write(chr(int("0B", 16)))
# 					code_idx += 2
# 				if isize % 2:
# 					fd.write(code[code_idx])
# 					code_idx += 1
		# to_store.append((func_name, address, code, is_is, inst_sizes))

fd = open(vmfile, 'r+b')

with open("dt_func_code_original_backup", 'w') as fd2:
	fd.seek(file_offset)
	for i in xrange(text_size):
		byte = fd.read(1)
		fd2.write(byte)

fd.close()

fd = open(iis, 'r+b')

with open("dt_func_code_is_backup", 'w') as fd2:
	# fd.seek(0x200000)
	fd.seek(file_offset)
	for i in xrange(text_size):
		byte = fd.read(1)
		fd2.write(byte)

fd.close()

fd = open(oos, 'r+b')

with open("dt_func_code_oos_backup", 'w') as fd2:
	# fd.seek(0x200000)
	fd.seek(file_offset)
	for i in xrange(text_size):
		byte = fd.read(1)
		fd2.write(byte)
		# print hex(ord(byte))

fd.close()

# with open("/home/muhammad/testing/debloating/dt_func_info_is", 'w') as fd:
# 	for (func_name, address, code, is_is, inst_sizes) in to_store:
# 		address = struct.pack("Q", int(address, 16))
# 		code_size = struct.pack("I", len(code))
# 		strlen = struct.pack("I", len(func_name))
# 		inst_size0 = struct.pack("I", inst_sizes[0])
# 		inst_size1 = struct.pack("I", inst_sizes[1])
# 		new_code = ""
# 		fd.write(strlen)
# 		fd.write(func_name)
# 		fd.write(address)
# 		fd.write(code_size)
# 		fd.write(inst_size0)
# 		fd.write(inst_size1)		
# 		fd.write(code)
