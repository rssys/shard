#!/bin/bash -e

echo "[ADIL] This script does not seem to work, please check"

cp ${SHARD_PATH}/guest-kernel/kernel-bitcode/bitcode-build-full-link/kernel-linked-bcs.o.bc .
cp ${SHARD_PATH}/guest-kernel/protected-kernel/kernel-linked-bcs.o.bc ${SHARD_PATH}/guest-kernel/cfi/.
cd ${SHARD_PATH}/guest-kernel/cfi
./build_and_run.sh kernel-linked-bcs.o
cp kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/protected-kernel/.

cp ${SHARD_PATH}/guest-kernel/protected-kernel/kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/specialization/kernel-linked-bcs.o.bc
cd ${SHARD_PATH}/guest-kernel/specialization
make
llvm-link kernel-linked-bcs.o.bc sc_funcs.bc spec_funcs.bc timing.bc -o kernel-linked-bcs.o.bc
./AddSCFuncs.sh kernel-linked-bcs.o kernel-linked-bcs.o-instrumented
cp kernel-linked-bcs.o-instrumented.bc kernel-linked-bcs.o.bc
./AddDPFuncs.sh kernel-linked-bcs.o kernel-linked-bcs.o-instrumented
cp kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/protected-kernel/.

cp ${SHARD_PATH}/guest-kernel/protected-kernel/kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/shadowstack/kernel-linked-bcs.o.bc
cd ${SHARD_PATH}/guest-kernel/shadowstack
make
llvm-link kernel-linked-bcs.o.bc ss_funcs.bc -o kernel-linked-bcs.o.bc
./add_protection.sh kernel-linked-bcs.o
cp kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/protected-kernel/.


cp ${SHARD_PATH}/guest-kernel/protected-kernel/kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/padding/kernel-linked-bcs.o.bc
cd ${SHARD_PATH}/guest-kernel/padding
make
llvm-link kernel-linked-bcs.o.bc padding_func.bc -o kernel-linked-bcs.o.bc
opt -load /home/muhammad/Repositories/padding/lib/AddPadding.so -add-padding -padding-bytes ${1} kernel-linked-bcs.o.bc -o kernel-linked-bcs.o-instrumented.bc
cp kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/protected-kernel/.

cp kernel-linked-bcs.o-instrumented.bc ${SHARD_PATH}/guest-kernel/kernel-bitcode/bitcode-build-full-link/.
cd ${SHARD_PATH}/guest-kernel/kernel-bitcode/bitcode-build-full-link/
./compile_occam_final.sh
cp vmlinux ${SHARD_PATH}/guest-kernel/protected-kernel/.
cd ${SHARD_PATH}/guest-kernel/protected-kernel/
objdump -d vmlinux &> vmlinux_dumped