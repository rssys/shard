#!/bin/bash -e

# copy the generated bitcode of the kernel
cp ../kernel-bitcode/bitcode-build-full-link/kernel-linked-bcs.o.bc ./
cp kernel-linked-bcs.o.bc ${SHARD_PATH}/fine-cfg/bcs/.
echo "[*] Copied the kernel bitcode"

cd ${SHARD_PATH}/fine-cfg/
./run_on_linux.sh kernel-linked-bcs.o.bc

cp tests/linux/linux_call_graph_fcfg ${SHARD_PATH}/guest-kernel/callgraph/.
cp tests/linux/linux_call_graph_sig ${SHARD_PATH}/guest-kernel/callgraph/.
cd ${SHARD_PATH}/guest-kernel/callgraph/.
sed -e '1,4d' < linux_call_graph_fcfg &> tmp && cat tmp &> linux_call_graph_fcfg && head -n -4 linux_call_graph_fcfg &> tmp && cat tmp &> linux_call_graph_fcfg
sed -e '1,4d' < linux_call_graph_sig &> tmp && cat tmp &> linux_call_graph_sig && head -n -1 linux_call_graph_sig &> tmp && cat tmp &> linux_call_graph_sig
python create_targets.py linux_call_graph_sig linux_call_graph_fcfg vmlinux &> callsites_info
sed -e '1,2d' < callsites_info &> tmp && cat tmp &> callsites_info
cp callsites_info ${SHARD_PATH}/guest-kernel/instrumentation/cfi/.

echo "[*] Script successfully completed!"