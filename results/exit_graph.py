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

def format_latex(s):
	idx = s.find("_")
	if idx < 0:
		return s
	return s[:idx] + '''\\''' + s[idx:]


is_redis = True
# is_redis = False

lines_orig = read_file(sys.argv[1])
names = map(lambda x : format_latex(x.split(' ')[0]), lines_orig)
medians = map(lambda x : float(x.split(' ')[1]), lines_orig)
print medians

lines_exits = read_file(sys.argv[2])
total = map(lambda x : float(x.split(' ')[1]), lines_exits)
cs = map(lambda x : float(x.split(' ')[2]), lines_exits)
hardening = map(lambda x : float(x.split(' ')[3].strip()) * 2, lines_exits)
print "hardening", np.mean(hardening)/2, hardening

print total
print cs

total = [x/y for x, y in zip(total, medians)]
cs = [x/y for x, y in zip(cs, medians)]

bar_width = 0.2

font = {'size'   : 30}
plt.rc('font', **font)
plt.rc('text', usetex=True)
plt.rc('font', family='sans-serif')

if is_redis:
	fig, ax = plt.subplots(figsize=(16, 8))
else:
	fig, ax = plt.subplots(figsize=(16, 6.6))

index = np.arange(len(total))
bar0 = ax.bar(index - bar_width, hardening,
                 bar_width, label="Hardening")
bar1 = ax.bar(index, cs,
                 bar_width, label="EPT Switches")
bar2 = ax.bar(index + bar_width, total,
                 bar_width, label="Total Traps")

ax.set_xticks(np.arange(len(names)) + 0.5 * bar_width)

ax.legend(ncol=3, loc='upper right').draggable()
if not is_redis:
	ax.set_xlabel("File Size (KB)")
	ax.set_xticklabels(names, rotation=0)
	# plt.ylim([0, 60000])
else:
	ax.set_xticklabels(names, rotation=80)

ax.set_ylabel("Number of exits (/sec)")
plt.tight_layout(0)
ax.xaxis.set_tick_params(length=0)
plt.ylim([1, 10**6])
ax.set_yscale('log')

plt.show()
fig.savefig(sys.argv[3], bbox_inches='tight')
