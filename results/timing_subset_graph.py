from matplotlib.ticker import FuncFormatter
import matplotlib.pyplot as plt
import numpy as np
import sys

# is_redis = False
is_redis = True

# is_wrk = True
is_wrk = False

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]


def tokenize_line(line):
	# print line
	ls = filter(lambda x : x != '', line.split(' '))
	ls = map(lambda x : x.strip(), ls)
	return ls

def format_latex(s):
	idx = s.find("_")
	if idx < 0:
		return s
	return s[:idx] + '''\\''' + s[idx:]

if is_wrk:
	mapping = {"sc_cs_hd" : r"SHARD", "on_ab" : r"SHARD-Prof$_{ab}$", "on_redis" : r"SHARD-Prof$_{Redis}$", "on_wrk" : r"SHARD"}
elif not is_redis:
	mapping = {"sc_cs_hd" : r"SHARD", "on_ab" : r"Prof$_{NGINX}$", "on_redis" : r"SHARD-Prof$_{Redis}$", "on_wrk" : r"Prof$_{wrk}$"}
else:
	mapping = {"sc_cs_hd" : r"SHARD", "subset" : r"SHARD-Prof$_{part}$", "on_redis" : r"Prof$_{Redis}$", "on_wrk" : r"Prof$_{wrk}$"}
# print mapping["ctx_cs_nhd"]

lines = read_file(sys.argv[1])
line = lines[0]
toks = tokenize_line(line)
x_name = toks[0]
key_labels = toks[1:]
lines = lines[1:]
toks =  map(list, zip(*map(tokenize_line, lines)))
x_ticks = toks[0]
vals = toks[1:]
# x_ticks = map(int, x_ticks)
vals = [map(lambda val_ls : float(val_ls) - 100.0, val_ls) for val_ls in vals]
print vals

index = np.arange(len(x_ticks))
if is_redis:
	bar_width = 0.25
else:
	bar_width = 0.25
font = {'size'   : 30}
plt.rc('font', **font)
plt.rc('text', usetex=True)
plt.rc('font', family='sans-serif')

if is_redis:
	fig, ax = plt.subplots(figsize=(16, 7.9))
else:
	fig, ax = plt.subplots(figsize=(16, 6.6))	
num = 0
cols = ['darkorange', 'purple']
for idx, ls in enumerate(vals):
	name = key_labels[idx][:-3].strip()
	print name, (name in mapping)
	if not name in mapping:
		continue
	print "printing", name, np.mean(ls), ls
	bar = ax.bar(index + num * bar_width, ls, bar_width, label=mapping[name], color=cols[num])
	num += 1

ax.set_xticks(np.arange(len(x_ticks)))
x_ticks = map(format_latex, x_ticks)
if is_redis:
	plt.ylim([0, 25])
	ax.set_xticklabels(x_ticks, rotation=80)
else:
	if not is_wrk:
		plt.ylim([0, 100])
	else:
		plt.ylim([0, 45])
	ax.set_xticklabels(x_ticks, rotation=0)
ax.xaxis.set_tick_params(length=0)
ax.legend(ncol=3, loc='upper right').draggable()
if not is_redis:
	ax.set_xlabel(' '.join(x_name.split('-')))
ax.set_ylabel("Overhead (\%)")
plt.tight_layout(0)
plt.show()
fig.savefig(sys.argv[2], bbox_inches='tight')