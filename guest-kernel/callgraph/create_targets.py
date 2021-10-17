import subprocess
import json
import os, sys
import ntpath
import shutil
from shutil import copyfile
import stat
import Queue
import re
import struct

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

def get_func_to_addr_mapping(vmfile):
	mapping = {}
	p1 = subprocess.Popen(["llvm-nm", vmfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "t "], stdin=p1.stdout, stdout=subprocess.PIPE)
	lines = p2.communicate()[0].split('\n')[:-1]
	for line in lines:
		func = line.split(' ')[2]
		addr = int(line.split(' ')[0], 16)
		mapping[func] = addr
	p1 = subprocess.Popen(["llvm-nm", vmfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "T "], stdin=p1.stdout, stdout=subprocess.PIPE)
	lines = p2.communicate()[0].split('\n')[:-1]
	for line in lines:
		func = line.split(' ')[2]
		addr = int(line.split(' ')[0], 16)
		mapping[func] = addr
	p1 = subprocess.Popen(["llvm-nm", vmfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "W "], stdin=p1.stdout, stdout=subprocess.PIPE)
	lines = p2.communicate()[0].split('\n')[:-1]
	for line in lines:
		func = line.split(' ')[2]
		addr = int(line.split(' ')[0], 16)
		mapping[func] = addr	
	p1 = subprocess.Popen(["llvm-nm", vmfile], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(["grep", "w "], stdin=p1.stdout, stdout=subprocess.PIPE)
	lines = p2.communicate()[0].split('\n')[:-1]
	for line in lines:
		func = line.split(' ')[2]
		addr = int(line.split(' ')[0], 16)
		mapping[func] = addr
	return mapping

def func_ci_targets(lines):
	nodes = {}
	num_calls = 0
	num_funcs = 0
	ind_targets = []
	num_unhandled = 0
	direct = 0
	for line in lines:
		if len(line) > 1 and line[-1] == "{" and line.find("callsite") < 0:
			num_funcs += 1
			currfunc = line[:-1]
			num_targets = 0
			nodes[currfunc] = []
		elif line == '}' or line == "__External_Function__":
			continue
		else:
			if not num_targets:
				num_calls += 1
				toks = line.split(' ')
				num_targets = int(toks[1][:-1])
				if toks[0] == "callsite:direct":
					curr_ci = "direct:" + str(direct)
					direct += 1
				else:
					if toks[0].split(':')[0] != "callsite":
						print "ERROR", line, toks[0].split(':')[0]
					idx = int(toks[0].split(':')[2])
					curr_ci = "indirect:" + str(idx)
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
			if not f in nodes:
				continue
			if not f in added:
				added.add(f)
				worklist.put(f)	
	return list(added)

def write_name(fd, name):
	strlen = struct.pack("I", len(name))
	fd.write(strlen)
	fd.write(name)

def print_ici(ci, targets):
	toks = ci.split(':')
	if toks[0] == "direct":
		return
	print ci 
	if len(targets) > 1000:
		return
	for target in targets:
		print target
lines_sig = read_file(sys.argv[1])
lines_fcfg = read_file(sys.argv[2])
fc_sig = func_ci_targets(lines_sig)
fc_fcfg = func_ci_targets(lines_fcfg)

payloads = ["get_root_af_packet", "get_root_af_packet_ss_1", "get_root_af_packet_ss_2", "get_root_fork"]

nodes = {}
for func in fc_sig:

	nodes[func] = set()
	calls_sig = fc_sig[func]
	calls_fcfg = fc_fcfg[func]
	if len(calls_sig) != len(calls_fcfg):
		print "ERROR :", len(calls_sig), len(calls_fcfg)
	for idx, call in enumerate(calls_fcfg):
		if call == "unhandled":
			call = calls_sig[idx]
		ci, targets = call
		for payload in payloads:
			if payload in targets:
				targets.remove(payload)
		print_ici(ci, targets)
total = sum([len(nodes[x]) for x in nodes])