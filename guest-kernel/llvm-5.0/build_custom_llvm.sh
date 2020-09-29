#!/bin/bash -e

# wget https://github.com/llvm/llvm-project/releases/download/llvmorg-5.0.0/llvm-5.0.0.src.tar.xz
# wget https://github.com/llvm/llvm-project/releases/download/llvmorg-5.0.0/clang-5.0.0.src.tar.xz

# tar xf llvm-5.0.0.src.tar.xz
# tar xf clang-5.0.0.src.tar.xz
# mv clang-5.0.0.src llvm-5.0.0.src/tools/clang

tar -xvf clang-5.src.tar.xz
mv -f cfe-5.0.0rc5.src llvm-5.0.0.src/tools/clang
./rebuild.sh