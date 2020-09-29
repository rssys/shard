import subprocess
import json
import os, sys
import ntpath
import shutil
from shutil import copyfile
import stat
import Queue
import re

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

def mkdir(work_dir):
	if os.path.exists(work_dir):
		shutil.rmtree(work_dir)
	os.makedirs(work_dir)

def write_to_file(name, data):
	with open(name, 'w') as fd:
		fd.write(data)

def func_ci_targets(lines):
	nodes = {}
	num_calls = 0
	num_funcs = 0
	ind_targets = []
	num_unhandled = 0
	for line in lines:
		if len(line) > 1 and line[-1] == "{":
			num_funcs += 1
			currfunc = line[:-1]
			num_targets = 0
			nodes[currfunc] = []
		elif line == '}' or line == "__External_Function__":
			continue
		else:
			if not num_targets:
				if not (line.find(" call ") >= 0 or (len(line) > 5 and line[:5] == "call ")):
					print line
				num_calls += 1
				num_targets = int(line.split(' ')[-1])
				curr_ci = ' '.join(line.split(' ')[:-1])
				currset = set()
				if num_targets == -1:
					nodes[currfunc].append("unhandled")
					num_targets = 0
					num_unhandled += 1
				elif num_targets == 0:
					nodes[currfunc].append((curr_ci, currset))
				if num_targets <= 1:
					continue
				ind_targets.append(num_targets)
			else:
				currset.add(line)
				num_targets -= 1
				if not num_targets:
					nodes[currfunc].append((curr_ci, currset))
	print num_unhandled
	return nodes

def get_deps(node, nodes):
	worklist = Queue.Queue()
	added = set()
	not_found = set()
	added.add(node)
	worklist.put(node)
	while not worklist.empty():
		worker = worklist.get()
		ls = nodes[worker]
		for f in ls:
			if f == "__External_Function__":
				continue
			if not f in nodes:
				continue
			if not f in added:
				added.add(f)
				worklist.put(f)	
	return list(added)

lines_fcfg = read_file(sys.argv[1])
fci_fcfg = func_ci_targets(lines_fcfg)
lines_pts_to = read_file(sys.argv[2])
fci_pts_to = func_ci_targets(lines_pts_to)

# call_targets = []
# for func in fci_pts_to:
# 	for tset in fci_pts_to[func]:
# 		call_targets.append(len(tset))
# call_targets.sort()
# print call_targets

nodes = {}
broken = False
num_fcfg = 0
num_pt = 0
num_total = 0
num_different = 0
for func in fci_fcfg:
	if func == "___bpf_prog_run":
		continue
	if not func in fci_pts_to:
		print func
	fcfg_calls = fci_fcfg[func]
	pt_calls = fci_pts_to[func]
	if not len(fcfg_calls) == len(pt_calls):
		print func
	for idx, (_, pt_set) in enumerate(pt_calls):
		val = fcfg_calls[idx]
		if val == "unhandled":
			continue
		ci, fcfg_set = val
		if len(pt_set) >= 6000:
			continue
		if len(pt_set) <= 1:
			continue
		num_not_present = 0
		for cf in pt_set:
			if not cf in fcfg_set:
				num_not_present += 1
				print "not present", cf
		if num_not_present:
			print func, ci, len(pt_set), num_not_present
			print fcfg_set
			num_different += 1
		num_total += 1
		# if len(pt_set) > len():
		# 	print func, idx
	# 		# temp_set = fcfg_calls[idx]
	# 	for edge in temp_set:
	# 		currset.add(edge)
	# nodes[func] = currset

print num_different, num_total

# ind_targets.sort()
# print ind_targets
# print sum(ind_targets) / len(ind_targets)

# num_edges = [len(nodes[node]) for node in nodes]
# num_edges.sort()
# print num_edges
# print sum(num_edges) / len(num_edges)



# sys_call_table = read_file("sys_call_table")[0]
# matched = re.findall("@([_a-zA-Z0-9]+)", sys_call_table)[1:]
# sys_calls = list(set(matched))
# print len(sys_calls)
# mkdir("syscall_deps")
# os.chdir("syscall_deps")
# num_deps = []
# ins = None
# for sys_call in sys_calls:
# 	name = sys_call
# 	if not name in nodes:
# 		name = "SyS_" + name[4:]
# 		if not name in nodes:
# 			print name, "NOT FOUND"
# 			continue
# 	deps = get_deps(name, nodes)
# 	print name, len(deps)
# 	num_deps.append(len(deps))
# 	write_to_file(name, '\n'.join(deps))
# 	if len(deps) < 980:
# 		continue
# 	if ins == None:
# 		ins = set(deps)
# 	else:
# 		ins = ins.intersection(deps)

# num_deps.sort()
# print num_deps
# print sum(num_deps) / len(num_deps)
# print len(num_deps), len(filter(lambda x : x > 980, num_deps))
# print len(ins)


# for node in nodes:
# 	print node,
# 	for edge in nodes[node]:
# 		print edge,
# 	print
