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

lines_orig = read_file(sys.argv[1])
medians = map(lambda x : float(x.split()[1]), filter(lambda x : x.find("median") >= 0, lines_orig))

lines_exits = read_file(sys.argv[2])
total = map(lambda x : float(x.split()[3])/10, filter(lambda x : x.find("num_handled") >= 0, lines_exits))
ctx = map(lambda x : float(x.split()[3])/10, filter(lambda x : x.find("num_switch_ctx_insts") >= 0, lines_exits))
sc = map(lambda x : float(x.split()[3][:-1])/10, filter(lambda x : x.find("num_sc_entry") >= 0, lines_exits))
cs = map(lambda x : get_cs(x)/10, filter(lambda x : x.find("num_sc_changes_in_scope") >= 0, lines_exits))

total = [x/y for x, y in zip(total, medians)]
print ctx
ctx = [x/y for x, y in zip(ctx, medians)]
print ctx
sc = [x/y for x, y in zip(sc, medians)]
cs = [x/y for x, y in zip(cs, medians)]

file_sizes = ["1", "2", "4", "8", "16", "32", "64", "128"]

index = np.arange(len(total))

bar_width = 0.15

font = {'size'   : 30}
plt.rc('font', **font)
plt.rc('text', usetex=True)
plt.rc('font', family='sans-serif')

fig, ax = plt.subplots(figsize=(16, 8))

# bar1 = ax.bar(index - bar_width, ctx,
#                  bar_width, label="context switches")
# bar2 = ax.bar(index, sc,
#                  bar_width, label="syscalls")

bar4 = ax.bar(index, total, bar_width,
                label="total exits")

ax.set_xticks(np.arange(len(file_sizes)) + 0.5 * bar_width)
ax.set_xticklabels(file_sizes, rotation=0)

ax.legend(ncol=1, loc='upper right').draggable()
ax.set_xlabel("File Size (KB)")
ax.set_ylabel("Number of exits (/sec)")
plt.tight_layout(2)
ax.xaxis.set_tick_params(length=0)

plt.show()
print sys.argv
fig.savefig(sys.argv[3], bbox_inches='tight')
