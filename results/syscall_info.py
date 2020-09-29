import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import numpy as np
import matplotlib.pyplot as plt

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]


service_syscalls = {"file_device" : [0, 1, 2, 3, 4, 5, 8, 16, 17, 18, 20, 33, 40, 72, 78, 79, 83, 89, 95], "process_control" : [9, 10, 11, 12, 13, 14, 16, 21, 28, 39, 56, 105, 106, 107, 110, 112, 116, 130, 157, 158, 218, 231, 302], "ipc" : [22, 40, 41, 42, 43, 45, 49, 50, 52, 53, 54, 202, 213, 232, 233, 257, 273, 288, 290], "info" : [63, 99]}
syscall_to_service = {}
for key in service_syscalls:
	for sc in service_syscalls[key]:
		syscall_to_service[sc] = key

def write_intersections(syscall_funcs, service1, service2):
	intersections = []
	with open("/home/muhammad/Repositories/kernelguard/syscall_intersections/" + service1 + '+' + service2, 'w') as fd:
		for sc in syscall_funcs:
			if syscall_to_service[sc] == service1:
				for sc2 in syscall_funcs:
					if sc == sc2:
						continue
					if syscall_to_service[sc2] == service2:
						ins = syscall_funcs[sc].intersection(syscall_funcs[sc2])
						intersections.append(len(ins))
						fd.write(str(sc) + ' ' + str(sc2) + " --> " + str(intersections[len(intersections) - 1]))
						for func in ins:
							fd.write(' ' + func)
						fd.write("\n")

		if not len(intersections):
			mn = -1
			mx = -1
		else:
			mn = np.mean(intersections)
			mx = np.max(intersections)
		fd.write(str(mn))
		fd.write("\n")
		fd.write(str(mx))
		fd.write("\n")

def format_name_latex(name):
	if name[:4] == "sys_" or name[:4] == "SyS_":
		name = name[4:]
	idx = name.find('_')
	if idx < 0:
		return name
	name = name[:idx] + '\\' + name[idx:] 
	return name

def get_func_info(vmlinux_dump, funcs):
	lines = read_file(vmlinux_dump)
	func_ints = {}
	funcs = set(funcs)
	pattern = re.compile("([a-z0-9]+) <([._a-zA-Z0-9]+)>:")
	inside_func = False
	for idx, line in enumerate(lines):
		if line == "":
			if inside_func:
				print num_insts
			inside_func = False
			continue
		matched = pattern.match(line)
		if matched:
			print line
			func_name = matched.group(2)
			address = int(matched.group(1), 16)
			if not func_name in funcs:
				continue
			num_insts = 0
		else:
			print line.split("\t")
			num_insts += 1
	return func_info

lines = read_file(sys.argv[1] + "_profile")
vmlinux_dump = sys.argv[4]
syscall_funcs = {}
funcs = set()
for line in lines:
	sc, func = line.split(':')
	sc = int(sc)
	if sc == -1:
		continue
	if not sc in syscall_funcs:
		syscall_funcs[sc] = set()
	funcs.add(func)
	syscall_funcs[sc].add(func)

syscall_num_funcs = [len(syscall_funcs[sc]) for sc in syscall_funcs]

func_info = get_func_info(vmlinux_dump, funcs)

# for service in service_syscalls:
# 	for service2 in service_syscalls:
# 		write_intersections(syscall_funcs, service, service2)

print "system calls", len(funcs)
print "mean", round(float(np.mean(syscall_num_funcs))/len(funcs), 4)
print "median", round(float(np.median(syscall_num_funcs))/len(funcs), 4)
print "max", np.max(syscall_num_funcs)
syscall_num_funcs = [round(float(x)/len(funcs), 2) for x in syscall_num_funcs]
syscall_num_funcs.sort(reverse=True)
print "80%", syscall_num_funcs[int(len(syscall_num_funcs) * 0.2) + 1], int(len(syscall_num_funcs) * 0.2) + 1

ls_syscall_funcs = [(sc, len(syscall_funcs[sc])) for sc in syscall_funcs]
ls_syscall_funcs.sort(key=lambda x : x[1], reverse=True)

sys_call_table = read_file("sys_call_table")[0]
matched = re.findall("@([_a-zA-Z0-9]+)", sys_call_table)[1:]
sys_calls = matched
# print sys_calls
for sc, ln in ls_syscall_funcs:
	print sc, sys_calls[sc], syscall_to_service[sc], ln, "|",
print

bound = int(sys.argv[3])
ls_syscall_funcs = ls_syscall_funcs[:bound]
matrix = [ [0] * bound for _ in range(bound) ]
for i, (sc1, ln1) in enumerate(ls_syscall_funcs):
	for j, (sc2, ln2) in enumerate(ls_syscall_funcs):
		insn = len(syscall_funcs[sc1].intersection(syscall_funcs[sc2]))
		ratio = max(float(insn)/ln1, float(insn)/ln2)
		print sys_calls[sc1], sys_calls[sc2], insn, i, j
		matrix[i][j] = insn
print sys_calls
print "",
for sc, _ in ls_syscall_funcs:
	print " & \\rotatebox[origin=lB]{90}{", format_name_latex(sys_calls[sc]) + "}",
print "\\\\"

shade = lambda x, y : int(np.round((float(x)/y) * 200))

for i, (sc, ln) in enumerate(ls_syscall_funcs):
	print format_name_latex(sys_calls[sc]),
	for j in xrange(bound):
		# if j > i:
		# 	continue
		if j == i:
			print " &", "\\cellcolor{blue!25}" + str(matrix[i][j]),
		else:
			print " &", "\\cca{" + str(shade(matrix[i][j], ln)) + "}", str(matrix[i][j]),
	print "\\\\"

# print syscall_funcs[9].intersection(syscall_funcs[41])

# font = {'family' : 'normal',
#         'weight' : 'bold',
#         'size'   : 20}

# plt.rc('font', **font)
# plt.rc('ytick', labelsize=15)    # fontsize of the tick labels
# fig, ax = plt.subplots()
# ax.plot(syscall_num_funcs, linewidth=3.0)
# # ax.set_title("Frequency distribution System Call functions  - " + sys.argv[2])
# ax.set_xticks(np.arange(len(syscall_num_funcs)))
# ax.set_xlabel("System Call")
# ax.set_ylabel("Required Functions")
# plt.tick_params(
#     axis='x',          # changes apply to the x-axis
#     which='both',      # both major and minor ticks are affected
#     bottom=False,      # ticks along the bottom edge are off
#     top=False,         # ticks along the top edge are off
#     labelbottom=False) # labels along the bottom edge are off
# fig.tight_layout()

# plt.show()
