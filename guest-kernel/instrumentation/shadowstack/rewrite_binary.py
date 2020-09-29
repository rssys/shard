import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat

file_offset = 0x200000 # 0x809000
text_size = 0x9a1000 # 12288
addr_lo = 0xffffffff81000000 # 0xffffffff81609000
addr_hi = addr_lo + text_size # 0xffffffff8160c000

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

def create_copy(binary, suff):
	cpy = binary + suff
	p = subprocess.Popen(['cp','-p','--preserve', binary, cpy])
	p.wait()
	return cpy

def get_new_call_offset(offset, line):
	toks = line.split('\t')
	inst_offset = int(toks[0][:-1], 16) + 5
	func_offset_bytes = toks[1].split(' ')[1:]
	func_offset_bytes.reverse()
	func_offset = int(''.join(func_offset_bytes), 16) + inst_offset
	return func_offset - (offset + 5)

def get_func_call_bytes(addr):
	addr = struct.pack("I", addr)
	return "\xe8" + addr

def get_relocation_pairs(binary):
	p = subprocess.Popen(["objdump", "-dF", binary.strip()], stdout=subprocess.PIPE)
	output = p.communicate()[0]
	output = output.split('\n')
	relocation_pairs = []
	pattern = re.compile("([a-z0-9]+) <([_.a-zA-Z0-9]+)> \(File Offset: ([a-z0-9]+)\):")
	curr_func_file_offset = -1
	curr_func_bin_offset = -1
	ss_exit_line = ""
	for idx, line in enumerate(output):
		line = line.strip()
		if line.find("callq") > 0 and line.find("ss_entry") > 0:
			toks = line.split('\t')
			# print line
			# print hex(curr_func_bin_offset)
			bytes_to_move = int(toks[0][:-1], 16) - curr_func_bin_offset
			new_inst_file_offset = curr_func_file_offset
			code_move_from_file_offset = new_inst_file_offset
			code_move_to_file_offset = new_inst_file_offset + 5
			new_inst_bin_offset = curr_func_bin_offset
			new_call_offset = get_new_call_offset(new_inst_bin_offset, line)
			move_info = (new_inst_file_offset, code_move_from_file_offset, code_move_to_file_offset, bytes_to_move, new_call_offset)	
			relocation_pairs.append(move_info)
			continue
		elif line.find("callq") > 0 and line.find("ss_exit") > 0:
			ss_exit_line = line
			continue
		elif line.find("retq") > 0:
			if ss_exit_line == "":
				continue
			toks = line.split('\t')
			ss_exit_func_offset = int(ss_exit_line.split('\t')[0][:-1], 16) - curr_func_bin_offset
			new_inst_func_offset = int(toks[0][:-1], 16) - curr_func_bin_offset - 5 # inst will occupy 6 insts
			new_inst_file_offset = curr_func_file_offset + new_inst_func_offset
			new_inst_bin_offset = curr_func_bin_offset + new_inst_func_offset
			bytes_to_move = (new_inst_func_offset + 5) - (ss_exit_func_offset + 5)
			code_move_from_file_offset = ss_exit_func_offset + curr_func_file_offset + 5
			code_move_to_file_offset = ss_exit_func_offset + curr_func_file_offset
			new_call_offset = get_new_call_offset(new_inst_bin_offset, ss_exit_line)
			move_info = (new_inst_file_offset, code_move_from_file_offset, code_move_to_file_offset, bytes_to_move, new_call_offset)
			relocation_pairs.append(move_info)
			ss_exit_line = ""
			continue
		matched = pattern.match(line)
		if not matched:
			continue
		curr_func_bin_offset = int(matched.group(1), 16)
		curr_func_file_offset = int(matched.group(3), 16)
	return relocation_pairs

def read_code(fname, offset, num_bytes):
	with open(fname, "r+b") as fd:
		fd.seek(offset)
		return fd.read(num_bytes)

def write_code(fname, offset, bytes):
	with open(fname, "r+b") as fd:
		fd.seek(offset)
		for b in bytes:
			fd.write(b)


binary = sys.argv[1]
relocation_pairs = get_relocation_pairs(binary)
new_copy = create_copy(binary, "_new")

for move_info in relocation_pairs:
	new_inst_file_offset, code_move_from_file_offset, code_move_to_file_offset, bytes_to_move, call_addr = move_info
	inst_bytes = get_func_call_bytes(call_addr)
	inst_len = 5
	func_bytes = read_code(new_copy, code_move_from_file_offset, bytes_to_move)
	# for b in func_bytes:
	# 	print hex(ord(b))
	# print "******************************************************"
	write_code(new_copy, new_inst_file_offset, inst_bytes)
	write_code(new_copy, code_move_to_file_offset, func_bytes)