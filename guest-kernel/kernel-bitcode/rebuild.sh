#!/bin/bash

cd linux-stable
git checkout v4.14.34
cp ../process_64.c arch/x86/kernel/process_64.c

# # Using the 4.14 version of the kernel. Versions up to 4.16 should work, but 4.17 does not
# # support clang 7 or under (TODO).
# # Create a .config file with the default configuration for the current architecture
# # It is possible to edit this configuration with make menuconfig or copy an existing one
# # make defconfig
cp ../miniconfig_x64-mod-yes .config

echo "Starting linux kernel build"

# make allmodconfig
# Building the kernel. A bug makes the build stop randomly with clang, so we start it
# again if it is not finished. However, this will loop if there is an actual error.
while [ ! -e "vmlinux" ]; do
    make -j`nproc` vmlinux CC=gclang
done

echo "[*] Linux kernel built successfully!"

# The python script will extract the bitcode to the specified folder and link it all in a new vmlinux object
# It needs the gclang log to be forwarded to this specific path. It can be changed in the python code. 
mkdir wrapper-logs
export WLLVM_OUTPUT_FILE=wrapper-logs/wrapper.log
mkdir ../bitcode-build-full-link
python ../built-in-parsing.py ../bitcode-build-full-link
# When using the modular script, the kernel will not boot if i use the bitcode for the drivers and ext4 file system

# This script created with the previous command will copy the bitcode files and link them
bash build_script.sh
cd ..
sed -i 's/kernel-linked-bcs.o.bc/kernel-linked-bcs.o-instrumented.bc/g' bitcode-build-full-link/link-args
sed -i 's/clang/${SHARD_PATH}\/guest-kernel\/llvm-5.0\/llvm-5.0.0.obj\/bin\/clang/g' bitcode-build-full-link/compile_occam_final.sh
sed -i 's/link-args-final/link-args/g' bitcode-build-full-link/compile_occam_final.sh

echo "[*] Script completed successfully!"
