import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile

file_offset = 0x200000 # 0x809000
text_size = 0x800000 # 12288
addr_lo = 0xffffffff81000000 # 0xffffffff81609000
addr_hi = addr_lo + text_size # 0xffffffff8160c000

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

def get_intr_exc_funcs(profile):
	irq_exc_funcs = set()
	with open(profile) as f:
	    for line in f:
			sc_id, func = line.strip().split(":")
			sc_id = int(sc_id)
			if sc_id < 0:
				irq_exc_funcs.add(func)
	return irq_exc_funcs

def get_addr_of(fname, sname):
	p1 = subprocess.Popen(["llvm-nm", fname], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", sname], stdin=p1.stdout, stdout=subprocess.PIPE)
	return p2.communicate()[0]


def get_bc_funcs(bcfile):
	# p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	# p2 = subprocess.Popen(["grep", "t "], stdin=p1.stdout, stdout=subprocess.PIPE)
	# funcs = [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	# p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	# p2 = subprocess.Popen(["grep", "T "], stdin=p1.stdout, stdout=subprocess.PIPE)
	# funcs += [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	# p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	# p2 = subprocess.Popen(["grep", "W "], stdin=p1.stdout, stdout=subprocess.PIPE)
	# funcs += [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	# p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	# p2 = subprocess.Popen(["grep", "w "], stdin=p1.stdout, stdout=subprocess.PIPE)
	# funcs += [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	# return funcs
	return set(read_file(bcfile))
def create_copy(vmfile, suff):
	cpy = vmfile + suff
	copyfile(vmfile, cpy)
	return cpy

def write_init_section_list(init_section_list):
	print len(init_section_list)
	with open("/home/muhammad/testing/debloating/init_section_list_2", 'w') as fd:
		for line, address, size in init_section_list:
			# print line, address, size
			address = struct.pack("Q", int(address, 16))
			size = struct.pack("I", size)
			fd.write(address)
			fd.write(size)

def get_frame_no(address, print_it):
	frame_no = address & 0xfffffffffffff000
	return frame_no


def overwrite_file(fname, func_info, irq_exc_funcs):
	to_store = []
	dt_funcs = ["ud2_call", "set_tracking", "sc_entry", "sc_exit", "ss_entry", "ss_exit", "save_old_ss", "add_new_ss", "printk", "log_fn", "getIdx", "__switch_to", "__switch_to_xtra", "do_syscall_64"]
	num_overwritten = 0
	with open(fname, "r+b") as fd:
		for (func_name, address, offset, inst_sizes) in func_info:
			if func_name in dt_funcs:
				continue
			if func_name in irq_exc_funcs:
				continue
			num_overwritten += 1
			offset = int(offset, 16)
			fd.seek(offset)
			code = fd.read(sum(inst_sizes))
			fd.seek(offset)
			code_idx = 0
			for isize in inst_sizes:
				if isize == 1: 
					fd.write(code[code_idx])
					code_idx += 1
					continue
				current = int(address, 16) + code_idx
				nex = int(address, 16) + code_idx + isize
				if get_frame_no(current, False) != get_frame_no(nex - 1, False):
					get_frame_no(current, True), get_frame_no(nex - 1, True)
					# print current, isize, "skipping address"
					for x in xrange(isize):
						fd.write(code[code_idx])
						code_idx += 1
					continue
				for x in xrange(isize/2):
					fd.write(chr(int("0F", 16)))
					fd.write(chr(int("0B", 16)))
					code_idx += 2
				if isize % 2:
					fd.write(chr(int("90", 16)))
					code_idx += 1
			to_store.append((func_name, address, code, inst_sizes))
		print "num_overwritten", num_overwritten
	return to_store

def write_ts_file(vmfile, ts_file):
	fd = open(vmfile, 'r+b')
	with open(ts_file, 'w') as fd2:
		# fd.seek(0x200000)
		fd.seek(file_offset)
		for i in xrange(text_size):
			byte = fd.read(1)
			fd2.write(byte)

	fd.close()

def get_symbol_info(vmfile, funcs):
	p = subprocess.Popen(["objdump", "-dF", vmfile.strip()], stdout=subprocess.PIPE)
	output = p.communicate()[0]
	output = output.split('\n')
	work_section = False
	func_info = []
	funcs = set(funcs)
	infunc = False
	init_section_list = []
	pattern = re.compile("([a-z0-9]+) <([._a-zA-Z0-9]+)> \(File Offset: ([a-z0-9]+)\):")
	for idx, line in enumerate(output):
		if line == "":
			infunc = False
			continue
		toks = line.split("\t")
		if work_section:
			if len(toks) >= 2:
				code_size = len(toks[1].split())
				if infunc:
					func_info[len(func_info) - 1][3].append(code_size)
				if line.find("_einittext") >= 0:
					init_section_list.append((line, line[:16], code_size))
		matched = pattern.match(line)
		if not matched:
			continue
		func_name = matched.group(2)
		address = int(matched.group(1), 16)
		if not func_name in funcs:
			continue
		if address < addr_lo:
			continue
		if address >= addr_hi:
			break
		work_section = True
		infunc = True
		address = matched.group(1)
		offset = matched.group(3)
		func_info.append((func_name, address, offset, []))
	return func_info, init_section_list


vmfile = sys.argv[1]
bcfile = sys.argv[2]
profile = sys.argv[3]
irq_exc_funcs = get_intr_exc_funcs(profile)
print len(irq_exc_funcs)
funcs = get_bc_funcs(bcfile)
func_info, init_section_list = get_symbol_info(vmfile, funcs)
# write_init_section_list(init_section_list)
ud2_file = create_copy(vmfile, "_ud2")
to_store = overwrite_file(ud2_file, func_info, irq_exc_funcs)
write_ts_file(ud2_file, "dt_func_code_ud2_backup")

with open("dt_func_info_is", 'w') as fd:
	prev_addr = -1
	prev_size = -1
	for (func_name, address, code, inst_sizes) in to_store:
		address = int(address, 16)
		if prev_addr > -1:
			if prev_addr >= address:
				print "prev_addr is not less than address", hex(prev_addr), hex(address)
		prev_addr = address
		prev_size = len(code)

		address = struct.pack("Q", address)
		code_size = struct.pack("I", len(code))
		strlen = struct.pack("I", len(func_name))
		inst_size0 = struct.pack("I", inst_sizes[0])
		if len(inst_sizes) == 1:
			inst_size1 = struct.pack("I", 0x0)
		else:
			inst_size1 = struct.pack("I", inst_sizes[1])
		fd.write(strlen)
		fd.write(func_name)
		fd.write(address)
		fd.write(code_size)
		fd.write(inst_size0)
		fd.write(inst_size1)		
		fd.write(code)