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

sys_call_table = read_file("sys_call_table")[0]
matched = re.findall("@([_a-zA-Z0-9]+)", sys_call_table)
syscalls = list(matched)

for idx, syscall in enumerate(syscalls):
	print idx, syscall
	if "sys_" + sys.argv[1] == syscall or "SyS_" + sys.argv[1] == syscall:
		print idx 
		break