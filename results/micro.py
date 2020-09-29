from matplotlib.ticker import FuncFormatter
import matplotlib.pyplot as plt
import numpy as np
import sys

# is_redis = False
is_redis = True

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


font = {'size'   : 30}
plt.rc('font', **font)
plt.rc('text', usetex=True)
plt.rc('font', family='sans-serif')

fig, ax = plt.subplots(figsize=(16, 6.6))	
bar_width = 0.5
bar = ax.bar(1 * bar_width, [1.75], bar_width, "original")
bar = ax.bar(1.5 * bar_width, [1.85], bar_width, "hardened")

ax.set_xticks(np.arange(len(x_ticks)))
x_ticks = map(format_latex, x_ticks)
if is_redis:
	plt.ylim([0, 15])
	ax.set_xticklabels(x_ticks, rotation=80)
else:
	plt.ylim([0, 50])
	ax.set_xticklabels(x_ticks, rotation=0)
ax.xaxis.set_tick_params(length=0)
ax.legend(ncol=3, loc='upper right').draggable()
if not is_redis:
	ax.set_xlabel(' '.join(x_name.split('-')))
ax.set_ylabel("Overhead (\%)")
plt.tight_layout(0)
plt.show()
fig.savefig(sys.argv[2], bbox_inches='tight')