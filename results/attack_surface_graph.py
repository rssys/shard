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

def format_latex(s):
	idx = s.find("_")
	if idx < 0:
		return s
	print idx, s
	return s[:idx] + '''\\_''' + format_latex(s[idx + 1:])

def get_ls(s):
	return s.replace(' ', '').replace('[', '').replace(']', '').replace("'", "").split(',')

lines = read_file("attack_surface_numbers")

x_labels = get_ls(lines[0])
lines = lines[1:]
nginx_1d_code = map(lambda x : float(x), get_ls(lines[0]))
redis_1d_code = map(lambda x : float(x), get_ls(lines[1]))
nginx_2d_code = map(lambda x : float(x), get_ls(lines[2]))
redis_2d_code = map(lambda x : float(x), get_ls(lines[3]))
sc_1d_code = map(lambda x : float(x), get_ls(lines[4]))
lines = lines[6:]
print lines[4]
nginx_1d_rop = map(lambda x : float(x), get_ls(lines[0]))
redis_1d_rop = map(lambda x : float(x), get_ls(lines[1]))
nginx_2d_rop = map(lambda x : float(x), get_ls(lines[2]))
redis_2d_rop = map(lambda x : float(x), get_ls(lines[3]))
sc_1d_rop = map(lambda x : float(x), get_ls(lines[4]))


font = {'size'   : 18, "family" : "Times New Roman"}
plt.rc('font', **font)
plt.rc('text', usetex=True)
plt.rc('font', family='sans-serif')

print len(nginx_2d_code), len(redis_2d_code), len(sc_1d_code)
print len(nginx_2d_rop), len(redis_2d_rop), len(sc_1d_rop)

# fig, (ax1,ax2) = plt.subplots(nrows=2, sharex=True, subplot_kw=dict(frameon=False)) # frameon=False removes frames

fig, (ax1, ax2) = plt.subplots(2, figsize=(20, 8))
x = np.arange(len(sc_1d_code))
# x = (np.arange(len(sc_1d_code), dtype=float) + 1)/len(sc_1d_code)
msize = 7
print x/len(sc_1d_code)
ax1.plot(x, nginx_2d_code, marker='D', markersize=msize, linewidth=3.0, label="NGINX$_{SHARD}$")
ax1.plot(x, redis_2d_code, marker='^', markersize=msize, linewidth=3.0, label="Redis$_{SHARD}$")
ax1.plot(x, sc_1d_code, marker='x', markersize=msize, linewidth=3.0, label="Assorted$_{SD}$")
ax1.plot(x, nginx_1d_code, marker='o', markersize=msize, linewidth=3.0, label="NGINX$_{AD}$")
ax1.plot(x, redis_1d_code, marker='s', markersize=msize, linewidth=3.0, label="Redis$_{AD}$")
# ax1.set_xticks([])
ax1.legend(ncol=5, loc='upper left').draggable()
ax1.set_ylabel("Assembly Instructions (\%)")
ax1.grid(which='major', axis='x')
ax1.axes.xaxis.set_visible(False)


ax2.plot(x, nginx_2d_rop, marker='D', markersize=msize, linewidth=3.0, label="Nginx$_{SHARD}$")
ax2.plot(x, redis_2d_rop, marker='^', markersize=msize, linewidth=3.0, label="Redis$_{SHARD}$")
ax2.plot(x, sc_1d_rop, marker='x', markersize=msize, linewidth=3.0, label="Assorted$_{SD}$")
ax2.plot(x, nginx_1d_rop, marker='o', markersize=msize, linewidth=3.0, label="Nginx$_{AD}$")
ax2.plot(x, redis_1d_rop, marker='s', markersize=msize, linewidth=3.0, label="Redis$_{AD}$")
ax2.set_xticks(x)

x_labels = map(format_latex, x_labels)
ax2.set_xticklabels(x_labels, rotation=90)
# ax2.legend(ncol=5, loc='upper left').draggable()
ax2.set_ylabel("ROP Gadgets (\%)")
# ax2.grid(b=None, which='major', axis='x')

plt.tight_layout(2)
ax1.set_xlim([0, len(sc_1d_code)])
ax2.set_xlim([0, len(sc_1d_code)])


xticks = ax2.get_xticks()
index = 0
for xtick in xticks:
	print ("hello")
	index += 1
	if index%10 == 0:
		ax1.vlines(xtick, 0, 20, linewidth=0.5)


xticks = ax2.get_xticks()
index = 0
for xtick in xticks:
	print ("hello")
	index += 1
	if index%10 == 0:
		ax2.vlines(xtick, 0, 20, linewidth=0.5)

plt.subplots_adjust(wspace=0, hspace=0)
fig.tight_layout(pad=1.5)

plt.show()
fig.savefig(sys.argv[1], bbox_inches='tight')
