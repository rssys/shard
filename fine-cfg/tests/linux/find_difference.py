import subprocess
import json
import os, sys
import ntpath
import shutil
from shutil import copyfile
import stat
import Queue
import re

def read_file(fname):
	with open(fname) as f:
		content = f.readlines()
	return [x.strip() for x in content]

set1 = set(read_file("unhandled"))
set2 = set(read_file("unhandled_2"))

for ci in set1:
	if not ci in set2:
		print ci