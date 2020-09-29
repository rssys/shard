opt -load ${SHARD_PATH}/guest-kernel/instrumentation/shadowstack/lib/SSProt.so -ss-protection ${1}.bc -o ${1}-instrumented.bc &> ss_funcs
