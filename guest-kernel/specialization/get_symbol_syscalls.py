import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat, numpy

printed = set()
with open(sys.argv[1]) as f:
	for line in f:
		sc_id, func = line.split(":")
		sc_id = int(sc_id)
		func = func.strip()
		if sc_id in printed:
			continue
		if func == sys.argv[2]:
			print sc_id, func
			printed.add(sc_id)
print len(printed)