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

callsites = []
for line in lines:
	if line.find(":") < 0:
		continue
	toks = line.split(':')
	if int(toks[1].split()[0]) == -1:
		continue
	callsites.append(int(toks[1].split()[1]))

callsites.sort()
print callsites