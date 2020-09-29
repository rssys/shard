from matplotlib.ticker import FuncFormatter
import matplotlib.pyplot as plt
import numpy as np
import sys

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]


def get_cs(line):
	toks = line.split()
	return float(toks[5][:-1]) + float(toks[7])

lines_exits = read_file(sys.argv[1])
total = map(lambda x : float(x.split()[3])/10, filter(lambda x : x.find("num_handled") >= 0, lines_exits))
cs = map(lambda x : get_cs(x)/10, filter(lambda x : x.find("num_sc_changes_in_scope") >= 0, lines_exits))

print total
print cs

names = [1, 2, 4, 8, 16, 32, 64, 128]
for idx, _ in enumerate(total):
	print names[idx], total[idx], cs[idx]
