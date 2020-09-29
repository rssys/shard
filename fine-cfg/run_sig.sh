make clean
make SigMatch
opt -load build/SigMatch.so -SigMatch ${1} -o bcs/tmp.bc 