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

def get_ls(s):
	return s.replace(' ', '').replace('[', '').replace(']', '').split(',')

def get_numbers(lines):
	nginx_app = map(lambda x : float(x), get_ls(lines[1]))[0]
	redis_app = map(lambda x : float(x), get_ls(lines[2]))[0]
	nginx = map(lambda x : (float(x)/nginx_app) * 100, get_ls(lines[3]))
	redis = map(lambda x : (float(x)/redis_app) * 100, get_ls(lines[4]))
	print nginx
	print redis
	nginx.sort(reverse=True)
	redis.sort(reverse=True)
	return nginx, redis
font = {'size'   : 30}
plt.rc('font', **font)
plt.rc('text', usetex=True)
plt.rc('font', family='sans-serif')

fig, ax = plt.subplots(figsize=(16, 8))
numbers1, numbers2 = get_numbers(read_file(sys.argv[1]))
x1 = (np.arange(len(numbers1), dtype=float) + 1)/len(numbers1)
x2 = (np.arange(len(numbers2), dtype=float) + 1)/len(numbers2)

ax.plot(x1, numbers1, linewidth=3.0, label="Nginx")
ax.plot(x2, numbers2, linewidth=3.0, label="Redis")
ax.legend(ncol=4, loc='upper right').draggable()
ax.set_xlabel("System Calls")
ax.set_ylabel("Required Instructions (\%)")
plt.tight_layout(2)
plt.show()
fig.savefig(sys.argv[2], bbox_inches='tight')
