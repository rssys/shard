import subprocess
import json
import os, sys
import ntpath
import shutil
from shutil import copyfile
import stat
import Queue
import re
import numpy

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

visited = set()
results = {}

def get_deps_rec(node, nodes):
	deps = set()
	if node == "__fdget_pos":
		return deps
	if node in visited:
		if node in results:
			return results[node]
		return deps # we are inside the call graph of node
	visited.add(node)
	deps.add(node)
	visited.add(node)
	for f in nodes[node]:
		child_deps = get_deps_rec(f, nodes)
		if node == "fput":
			print f, len(child_deps)
		for cd in child_deps:
			deps.add(cd)
	results[node] = deps
	return deps


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
			if f == "printk":
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
ind_targets = []
broken = False
num_fcfg = 0
num_pt = 0
num_total = 0
num_different = 0
for func in fci_fcfg:
	if not func in fci_pts_to:
		print func
	nodes[func] = set()
	fcfg_calls = fci_fcfg[func]
	pt_calls = fci_pts_to[func]
	if not len(fcfg_calls) == len(pt_calls):
		print func
	for idx, (_, pt_set) in enumerate(pt_calls):
		val = fcfg_calls[idx]
		edges = set()
		if len(pt_set) <= 6000:
			edges = pt_set
		else:
			if val == "unhandled":
				continue
			ci, fcfg_set = val
			#if len(fcfg_set) == 0 and len(pt_set) > 6000:
				#print ci
			edges = fcfg_set
		ind_targets.append(len(edges))
		for edge in edges:
			nodes[func].add(edge)

# ind_targets.sort()
# print ind_targets
# print len(filter(lambda x : x == 0, ind_targets))
# print sum(ind_targets) / len(ind_targets)

# num_edges = [len(nodes[node]) for node in nodes]
# num_edges.sort()
# print num_edges
# print sum(num_edges) / len(num_edges)

sys_call_table = read_file("sys_call_table")[0]
matched = re.findall("@([_a-zA-Z0-9]+)", sys_call_table)[1:]
sys_calls = list(set(matched))
print len(sys_calls)
mkdir("syscall_deps")
os.chdir("syscall_deps")
num_deps = []
ins = None
for sys_call in sys_calls:
	name = sys_call
	if not name in nodes:
		name = "SyS_" + name[4:]
		if not name in nodes:
			print name, "NOT FOUND"
			continue
	deps = get_deps(name, nodes)
	print name, len(deps)
	num_deps.append(len(deps))
	write_to_file(name, '\n'.join(deps))
	if len(deps) < 980:
		continue
	if ins == None:
		ins = set(deps)
	else:
		ins = ins.intersection(deps)

num_deps.sort()
num_large = filter(lambda x : x > 1200, num_deps)
print num_deps
print "total system calls", len(num_deps)
print "average number of targets for all system calls -", numpy.mean(num_deps)
print "median number of targets for all system calls -", numpy.median(num_deps)
print "total system calls with greater than 100 dependent functions", len(num_large)
print "intersection of dependencies for these system calls -", len(ins)
print "average targets for these system calls -", numpy.mean(num_large)
print "median number of targets for all system calls -", numpy.median(num_large)
print "more than 100 functions above the intersection - ", len(filter(lambda x : x < len(ins) + 100, num_large))

#print len(get_deps_rec("SyS_write", nodes))

# for node in nodes:
# 	print node,
# 	for edge in nodes[node]:
# 		print edge,
# 	print
