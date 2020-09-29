import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat, numpy

with open(sys.argv[1]) as f:
	for line in f:
		sc_id, func = line.split(":")
		sc_id = int(sc_id)
		func = func.strip()
		if sc_id == int(sys.argv[2]) and func == sys.argv[3]:
			print line,
			sys.exit(0)
print "not found"
