import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
from os import walk
import numpy

file_offset = 0x200000 # 0x809000
text_size = 0x800000 # 12288
addr_lo = 0xffffffff81000000 # 0xffffffff81609000
addr_hi = addr_lo + text_size # 0xffffffff8160c000
num_ts_frames = 2048

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

def get_bc_funcs(bcfile):
	p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "t "], stdin=p1.stdout, stdout=subprocess.PIPE)
	funcs = [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "T "], stdin=p1.stdout, stdout=subprocess.PIPE)
	funcs += [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "W "], stdin=p1.stdout, stdout=subprocess.PIPE)
	funcs += [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	p1 = subprocess.Popen(["llvm-nm", bcfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "w "], stdin=p1.stdout, stdout=subprocess.PIPE)
	funcs += [x.split(' ')[2] for x in p2.communicate()[0].split('\n')[:-1]]
	return funcs

def get_symbol_info(vmfile, funcs):
	p = subprocess.Popen(["objdump", "-dF", vmfile.strip()], stdout=subprocess.PIPE)
	output = p.communicate()[0]
	output = output.split('\n')

	work_section = False
	func_info = []
	funcs = set(funcs)
	to_exclude = ["total_mapping_size", "load_elf_binary", "chksum_update"]
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
		offset = matched.group(3)
		func_info.append((func_name, address, offset, []))

	fi_map = {}
	print len(func_info)
	global total_sizes
	total_sizes = 0
	for (func_name, address, _, inst_sizes) in func_info:
		num = 0
		if func_name == "SyS_ioctl":
			print sum(inst_sizes)
			print inst_sizes
		while func_name in fi_map:
			func_name += "_"
		fi_map[func_name] = (address, sum(inst_sizes))
		total_sizes += sum(inst_sizes)
	return fi_map, init_section_list

def read_sc_table():
	sys_call_table = read_file("sys_call_table")[0]
	matched = re.findall("@([_a-zA-Z0-9]+)", sys_call_table)
	syscalls = [sc for _, sc in enumerate(list(matched))]
	return syscalls

def get_intr_funcs(profile):
	irq_funcs = set()
	irq_funcs.add("irq_enter")
	irq_funcs.add("irq_exit")
	in_intr = False
	with open(profile) as f:
	    for line in f:
			sc_id, func = line.strip().split(":")
			sc_id = int(sc_id)
			if func == "irq_enter":
				in_intr = True
			elif func == "irq_exit":
				in_intr = False
			if in_intr:
				irq_funcs.add(func)
	return irq_funcs

def get_sc_info(sc_path, syscalls):
	sc_info = []
	for idx, sc in enumerate(syscalls):
		sc_info.append((sc, idx, set()))
	with open(sc_path) as fd:
	    for line in fd:
	    	line = line.strip()
	    	sc_id, func_name = line.split(":")
	    	sc_info[int(sc_id)][2].add(func_name)
	return sc_info

def write_sc_info(to_write):
	with open("sc_info", 'w') as fd:
		for sc, sc_id, frames in to_write:
			strlen = struct.pack("I", len(sc))
			fd.write(strlen)
			fd.write(sc)
			fd.write(struct.pack("I", sc_id))
			for frame_info in frames:
				fd.write(struct.pack("I", len(frame_info)))
				frame_info.sort()
				for start, end in frame_info:
					fd.write(struct.pack("I", start))
					fd.write(struct.pack("I", end))

def create_sc_dep_info(sc_info, func_info_map):
	to_write = []
	true_frames = []
	sizes = []
	for sc, sc_id, deps in sc_info:
		frames = []
		for frame in xrange(num_ts_frames):
			frames.append([])

		sc_size = 0
		empty = len(deps) == 0
		for func in deps:
			if not func in func_info_map:
				# print "skipping function", func
				continue
			addr, size = func_info_map[func]
			sc_size += size
			frame_begin = (addr - addr_lo) >> 12
			offset = (addr - addr_lo) - (frame_begin * 4096)
			frame_end = (addr + size - addr_lo) >> 12
			orig_size = size
			if sc_id == 16:
				print func, hex(addr), size
			for frame in range(frame_begin, frame_end + 1):
				# print frame
				if frame > num_ts_frames:
					continue
				start = offset
				end = min(offset + size, 4096)
				diff = end - offset
				size -= diff
				offset = 0 # always 0 after first time
				if sc_id == 16:
					print frame, start, end
				# if func == "commit_creds":
				# 	print func, hex(addr), orig_size
				# 	print start, end
				frames[frame].append((start, end))
				if sc_id == 16:
					print frames[0], frames[frame]
		true_frames.append(len(filter(lambda x : len(x) > 0, frames)))
		if sc_id == 16:
			print true_frames[-1]
		to_write.append((sc, sc_id, frames))
		# print "for syscall", sc, "id -", sc_id, "size is", sc_size, len(deps)
		# print "true frames are",
		# for idx, frame in enumerate(frames):
		# 	if frame:
		# 		print idx, 
		# print ""
		# print "******************************************************************************"
		sizes.append(sc_size)

	write_sc_info(to_write)
	true_frames.sort()
	true_frames = filter(lambda x : x > 0, true_frames)
	print len(true_frames)
	print "true_frames", true_frames
	print numpy.mean(true_frames)
	print "total true_frames", numpy.sum(true_frames)
	print numpy.median(true_frames)
	print numpy.mean(sizes)

vmfile = sys.argv[1]
bcfile = sys.argv[2]
sc_path = sys.argv[3]
funcs = get_bc_funcs(bcfile)
func_info_map, _ = get_symbol_info(vmfile, funcs)
syscalls = read_sc_table()
sc_info = get_sc_info(sc_path, syscalls)
# intr_funcs = get_intr_funcs(sc_path)
# print intr_funcs
create_sc_dep_info(sc_info, func_info_map)
print total_sizes