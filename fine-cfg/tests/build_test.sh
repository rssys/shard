gclang -g src/${2}.c -o objs/${2}
get-bc -b objs/${2}
opt -mem2reg objs/${2}.bc -o objs/${2}.opt
llvm-dis objs/${2}.opt
cd ..
./run_${1}.sh tests/objs/${2}.opt
