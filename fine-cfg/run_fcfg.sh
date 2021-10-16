make clean
make FineCFG
opt -load build/FineCFG.so -FineCFG ${1} -o ${1} 