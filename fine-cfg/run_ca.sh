make clean
make CheckApplicability
opt -load build/CheckApplicability.so -CheckApplicability ${1} -o bcs/tmp.bc 