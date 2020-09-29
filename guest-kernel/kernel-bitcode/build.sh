#!/bin/bash -e

# Cloning the Linux kernel source code
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git

# Call the rebuild script
./rebuild.sh
