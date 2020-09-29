from matplotlib.ticker import FuncFormatter
import matplotlib.pyplot as plt
import numpy as np
import sys

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

def getps(t, num):
	return round(100 * num/(t), 2)

def write_to_file(fd, ls):
	f = ''
	for i in range(len(ls)):
		f = f + '{:>15} '
	line_new = f.format(*ls)
	line_new += '\n'
	fd.write(line_new)

test_names = ["original", "our_baseline", "ctx_cs_nhd", "sc_ncs_nhd", "sc_cs_nhd", "sc_cs_hd"]
files = ["nginx_execution_log_" + x for x in test_names]
lines = map(read_file, files)
times = [filter(lambda x : len(x) > 6 and x[:6] == "median", test_lines) for test_lines in lines]
times = [map(lambda line : float(line.split(' ')[1]), time_tok) for time_tok in times]
orig_time = times[0]
spec_times = times[1:]
print spec_times[0]
print np.mean(spec_times[0]), "average 000"
spec_times = [[getps(a, b) for a, b in zip(orig_time, spec_time)] for spec_time in spec_times] 
fdt = open('nginx_times.csv','w')

ls = ["File Size (KB)"] + map(lambda x : x + "(%)", test_names[1:])
write_to_file(fdt, ls)
file_sizes = [1, 2, 4, 8, 16, 32, 64, 128]
print np.mean(spec_times[0]), spec_times[0]
for file_size, times in zip(file_sizes, zip(*spec_times)):
	write_to_file(fdt, [file_size] + list(times))

for idx, fs in enumerate(file_sizes):
	print fs, orig_time[idx]/2