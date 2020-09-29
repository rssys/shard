import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile

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

bc_funcs = get_bc_funcs(sys.argv[1])
lines = read_file(sys.argv[2])
syscalls_to_funcs = {}
irq_funcs = set()

irq_to_skip = ["irq_enter", "irq_exit", "log_irq_enter", "log_irq_exit"]
for line in lines:
	if line[:19] != "logging function : ":
		continue
	line = line[19:]
	sc_id, fname = line.split(':')
	sc_id = int(sc_id)
	if fname in irq_to_skip:
		continue
	if not fname in bc_funcs:
		print "skipping", line
		continue
	if sc_id < 0:
		irq_funcs.add(fname)
		continue
	if not sc_id in syscalls_to_funcs:
		syscalls_to_funcs[sc_id] = set()
	syscalls_to_funcs[sc_id].add(fname)

fd = open(sys.argv[3] + "_profile", "w")
for f in irq_funcs:
	fd.write("-1:" + f + "\n")
for f in irq_to_skip:
	print f
	fd.write("-1:" + f + "\n")
for sc_id in syscalls_to_funcs:
	for f in syscalls_to_funcs[sc_id]:
		fd.write(str(sc_id) + ":" + f + "\n")
fd.close()

fd = open(sys.argv[3] + "_stats", "w")
for sc_id in syscalls_to_funcs:
	fd.write(str(sc_id) + ":" + str(len(syscalls_to_funcs[sc_id])) + "\n")
fd.close()