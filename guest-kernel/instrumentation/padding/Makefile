LLVM_INCLUDE_PATH=/usr/include/llvm-5.0/
LLVM_C_INCLUDE_PATH=/usr/include/llvm-c-5.0/

all: build_AddPadding

build_AddPadding:
	mkdir -p lib
	clang -emit-llvm -fshort-wchar -o  padding_func.bc -c padding_func.c
	clang -I$(LLVM_INCLUDE_PATH) -I$(LLVM_C_INCLUDE_PATH) -I./ -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -ffunction-sections -fdata-sections -Wcast-qual -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcovered-switch-default -Wno-uninitialized -Wno-missing-field-initializers -Wno-comment -c -MMD -MP -MF "lib/AddPadding.d.tmp" -MT "AddPadding.o" -MT "AddPadding.d" AddPadding.cpp -o lib/AddPadding.o
	clang -O3 -Wl,-R -Wl,'$ORIGIN' -Wl,--gc-sections -rdynamic -L./ -Lm./ -shared -o lib/AddPadding.so lib/AddPadding.o -lpthread -ltinfo -ldl -lm
