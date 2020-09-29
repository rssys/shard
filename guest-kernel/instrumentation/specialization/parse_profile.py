import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat, numpy

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

syscall_deps = {}
with open(sys.argv[1]) as f:
    for line in f:
		sc_id, func = line.split(":")
		sc_id = int(sc_id)
		if not sc_id in syscall_deps:
			syscall_deps[sc_id] = set()
		syscall_deps[sc_id].add(func)

sys_call_table = read_file("sys_call_table")[0]
matched = re.findall("@([_a-zA-Z0-9]+)", sys_call_table)
syscalls = list(matched)
num_deps = []
funcs = set()
for idx, sys_call in enumerate(syscalls):
	if not idx in syscall_deps:
		continue
	num_deps.append(len(syscall_deps[idx]))
	for f in syscall_deps[idx]:
		funcs.add(f)
	# print sys_call, len(syscall_deps[idx])

num_deps.sort()
print num_deps
print "total system calls", len(num_deps)
print "average number of targets for all system calls -", numpy.mean(num_deps)
print "median number of targets for all system calls -", numpy.median(num_deps)
print "total number of required functions -", len(funcs)