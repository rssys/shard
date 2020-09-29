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


lines = read_file(sys.argv[1])
ci_indices = []
for line in lines:
	if line.find("indirect") < 0:
		continue
	idx = int(line.split(":")[1])
	ci_indices.append(idx)

with open("cfi_lib.h", 'w') as fd:
	fd.write("#define num_call_sites " + str(max(ci_indices) + 10) + "\n")
	fd.write("#define num_call_site_targets " + str(len(lines) + 10) + "\n")
	fd.write("#define num_ts_frames " + str(2048) + "\n")