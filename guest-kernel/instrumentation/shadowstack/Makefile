LLVM_INCLUDE_PATH=/usr/include/llvm-5.0/
LLVM_C_INCLUDE_PATH=/usr/include/llvm-c-5.0/

all: build_vulnerable build_SSProt
build_vulnerable:
	clang -emit-llvm -fshort-wchar -o vulnerable.bc -c vulnerable.c
	clang -emit-llvm -Wl,-T,vmlinux.lds,--whole-archive -Qunused-arguments -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 -fno-PIE -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -m64 -mno-80387 -mstack-alignment=8 -mtune=generic -mno-red-zone -mcmodel=kernel -funit-at-a-time -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -DCONFIG_AS_CFI_SECTIONS=1 -DCONFIG_AS_FXSAVEQ=1 -DCONFIG_AS_SSSE3=1 -DCONFIG_AS_CRC32=1 -DCONFIG_AS_AVX=1 -DCONFIG_AS_AVX2=1 -DCONFIG_AS_SHA1_NI=1 -DCONFIG_AS_SHA256_NI=1 -pipe  -fno-asynchronous-unwind-tables -O2 -fno-stack-protector  -mno-global-merge -no-integrated-as -fomit-frame-pointer  -fno-strict-overflow -fno-merge-all-constants -fno-stack-check -nostdlib -o ss_funcs.bc -c ss_funcs.c
	clang vulnerable.bc -o vulnerable
	llvm-link vulnerable.bc ss_funcs.bc -o vulnerable.bc

build_SSProt:
	mkdir -p lib
	clang -I$(LLVM_INCLUDE_PATH) -I$(LLVM_C_INCLUDE_PATH) -I./ -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -ffunction-sections -fdata-sections -Wcast-qual -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcovered-switch-default -Wno-uninitialized -Wno-missing-field-initializers -Wno-comment -c -MMD -MP -MF "lib/SSProt.d.tmp" -MT "SSProt.o" -MT "SSProt.d" SSProt.cpp -o lib/SSProt.o
	clang -O3 -Wl,-R -Wl,'$ORIGIN' -Wl,--gc-sections -rdynamic -L./ -Lm./ -shared -o lib/SSProt.so lib/SSProt.o -lpthread -ltinfo -ldl -lm
