#!/bin/bash -e

# wget https://github.com/llvm/llvm-project/releases/download/llvmorg-5.0.0/llvm-5.0.0.src.tar.xz
# wget https://github.com/llvm/llvm-project/releases/download/llvmorg-5.0.0/clang-5.0.0.src.tar.xz

# tar xf llvm-5.0.0.src.tar.xz
# tar xvf clang-5.src.tar.xz
# mv clang-5 llvm-5.0.0.src/tools/clang

cp InlineFunction.cpp llvm-5.0.0.src/lib/Transforms/Utils
cp TailRecursionElimination.cpp llvm-5.0.0.src/lib/Transforms/Scalar

mkdir llvm-5.0.0.obj || true
cd llvm-5.0.0.obj

cmake -DCMAKE_BUILD_TYPE=MinSizeRel  \
      -DLLVM_TARGETS_TO_BUILD="X86"  \
      ../llvm-5.0.0.src

make -j`nproc`