import subprocess, sys, json, os
import re
import sys
import struct
from shutil import copyfile
import stat, numpy

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

irq_funcs = set()
irq_funcs.add("irq_enter")
irq_funcs.add("irq_exit")
print "looping"
in_intr = False
with open(sys.argv[1]) as f:
    for line in f:
		sc_id, func = line.strip().split(":")
		sc_id = int(sc_id)
		if func == "irq_enter":
			in_intr = True
		elif func == "irq_exit":
			in_intr = False
		if in_intr:
			irq_funcs.add(func)
print irq_funcs
print len(irq_funcs)
