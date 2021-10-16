import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat

fname = sys.argv[1]
num_iters = int(sys.argv[2])

times = []

SHARD_HOME = os.getenv('SHARD_PATH')

for i in range(0, num_iters):
	print(fname)
	p = subprocess.Popen([SHARD_HOME + "/experimentation/nginx/client/ab_script", fname], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	output = p.communicate()[0]
	lines = output.decode().split('\n')
	#print(output)
	for line in lines:
		print(line)
		if len(line) > 15 and line[:15] == "Document Length":
			bytes = line.split(' ')[-2]
			print(bytes)
		if len(line) > 20 and line[:20] == "Time taken for tests":
			print(line)
			time = float(line.split(' ')[-2])
			times.append(time)

times.sort()

print("mean", sum(times)/len(times))
print("median", times[int(num_iters/2)])
