import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import numpy as np

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

def get_exp_data(exp):
	lines = read_file("output_" + exp)
	tests = {}
	for idx, line in enumerate(lines):
		if line.find("completed in") >= 0:
			time = float(line.split(' ')[-2])
			prev_line = lines[idx - 1]
			test_name = prev_line.split(' ')[1]
			if not test_name in tests:
				tests[test_name] = []
			# print tests	
			# print test_name, time
			tests[test_name].append(time)
	return tests

def format_line(name):
	idx = name.find("_")
	if idx < 0:
		return name
	return name[:idx] + "\_" + name[idx + 1:]

def write_to_file(fd, ls):
	f = ''
	for i in range(len(ls)):
		f = f + '{:>15} '
	line_new = f.format(*ls)
	line_new += '\n'
	fd.write(line_new)


experiments =  ["native", "no_log_fn", "no_log_fn_sc_cs_nhd", "no_log_fn_sc_cs_hd", "orig", "ctx_ncs_nhd", "ctx_cs_nhd", "sc_ncs_nhd", "sc_cs_nhd", "sc_cs_hd"]
exp_tests = {}

fd = open('redis_times.csv','w')
ls = ["Test Name"] + map(lambda x : x + "(%)", experiments[1:5])
write_to_file(fd, ls)
for idx, exp in enumerate(experiments):
	exp_tests[exp] = get_exp_data(exp)
overheads = {}
for test in exp_tests[experiments[0]]:
	baseline = np.median(exp_tests[experiments[0]][test])
	print test, baseline
	ls = [test]
	for idx, exp in enumerate(experiments[1:5]):
		if not test in exp_tests[exp]:
			prop = 100
			continue
		if idx == 0:
			res = exp_tests[exp][test]
			res.sort()
			prop = (res[-3] * 100)/baseline
		elif idx < 5:
			res = exp_tests[exp][test]
			res.sort()
			prop = (np.median(res) * 100)/baseline
			# print prop
			# print prop
		# else:
		# 	prop = np.median(exp_tests[exp][test]) * 100/baseline
		if not exp in overheads:
			overheads[exp] = []
		overheads[exp].append(prop - 100)
		ls.append(np.round(prop, 2))
	write_to_file(fd, ls)
	# print "\\\\"
	# print"\\hline"

fd.close()


# for exp in experiments:
# 	print exp, 
# print

print "average & 0", 
for exp in experiments[1:5]:
	print np.round(np.mean(overheads[exp]), 2),
print
print "\\\\"
print"\\hline"

