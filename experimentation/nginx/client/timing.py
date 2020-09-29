import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat

file = sys.argv[1]
num_iters = int(sys.argv[2])

times = []
for i in range(0, num_iters):
	p = subprocess.Popen(["/home/muhammad/Repositories/nginx/ab_script", file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	output = p.communicate()[0]
	lines = output.decode().split('\n')
	#print(output)
	for line in lines:
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
