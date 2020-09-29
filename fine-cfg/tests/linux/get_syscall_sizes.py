import subprocess
import json
import os, sys
import numpy
from operator import itemgetter

def write_to_file(fd, ls):
	f = ''
	for i in range(len(ls)):
		f = f + '{:>35} '
	line_new = f.format(*ls)
	line_new += '\n'
	fd.write(line_new)


def count_lines(fname):
	with open(fname) as f:
		for i, l in enumerate(f):
			pass
		return i + 1

scdir = sys.argv[1]

num_deps = []
for fname in os.listdir(scdir):
	num_deps.append((fname, count_lines(scdir + '/' + fname)))

num_deps = sorted(num_deps, key=itemgetter(1))

fd = open("syscall_dep_nums", 'w')
write_to_file(fd, ["system call", "number of dependencies"])
write_to_file(fd, ["\n", "\n"])

for nd in num_deps:
	write_to_file(fd, list(nd))