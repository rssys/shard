#!/bin/bash -e

# Cloning the Linux kernel source code
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
git checkout v4.14.34
cd ..

# Call the rebuild script
./rebuild.sh
