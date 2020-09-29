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

names = ["PING_INLINE", "PING_BULK", "MSET"]
for idx, _ in enumerate(total[:2]):
	if idx == 0:
		print names[idx], total[0]/2, cs[0]/2
		print names[idx + 1], total[0]/2, cs[0]/2
	else:
		print names[idx + 1], total[idx] - total[idx - 1], cs[idx] - cs[idx - 1]

t = total[2]
c = cs[2]

names = ["SPOP", "HSET", "SADD", "RPOP", "LPOP", "RPUSH", "LPUSH", "INCR", "GET", "SET"]
total = total[3:]
cs = cs[3:]
for idx, _ in enumerate(total):
	if idx == 0:
		print names[idx], total[idx], cs[idx]
	else:
		print names[idx], total[idx] - total[idx - 1], cs[idx] - cs[idx - 1]
		if names[idx] == "LPUSH":
			t -= total[idx] - total[idx - 1]
			c -= cs[idx] - cs[idx - 1]


names = ["LRANGE_100", "LRANGE_300", "LRANGE_500", "LRANGE_600"]

for name in names:
	print name, t/4, c/4