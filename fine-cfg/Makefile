LLVM_INCLUDE_PATH=${LLVM_SRC}/include/
LLVM_OBJ_INCLUDE_PATH=${LLVM_OBJ}/include/
LLVM_INCLUDE_PATH=/usr/include/llvm-5.0/
LLVM_C_INCLUDE_PATH=/usr/include/llvm-c-5.0/


all: FineCFG SigMatch

FineCFG:
	mkdir -p build
	clang -I$(LLVM_INCLUDE_PATH) -I$(LLVM_C_INCLUDE_PATH) -I./ -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -ffunction-sections -fdata-sections -Wcast-qual -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcovered-switch-default -Wno-uninitialized -Wno-missing-field-initializers -Wno-comment -c -MMD -MP -MF "build/Unroll.d.tmp" -MT "Unroll.o" -MT "Unroll.d" src/FineCFG.cpp -o build/FineCFG.o
	clang -O3 -Wl,-R -Wl,'$ORIGIN' -Wl,--gc-sections -rdynamic -L./ -Lm./ -shared -o build/FineCFG.so build/FineCFG.o -lpthread -ltinfo -ldl -lm

SigMatch:
	mkdir -p build
	clang -I$(LLVM_INCLUDE_PATH) -I$(LLVM_C_INCLUDE_PATH) -I./ -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -ffunction-sections -fdata-sections -Wcast-qual -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcovered-switch-default -Wno-uninitialized -Wno-missing-field-initializers -Wno-comment -c -MMD -MP -MF "build/Unroll.d.tmp" -MT "Unroll.o" -MT "Unroll.d" src/SigMatch.cpp -o build/SigMatch.o
	clang -O3 -Wl,-R -Wl,'$ORIGIN' -Wl,--gc-sections -rdynamic -L./ -Lm./ -shared -o build/SigMatch.so build/SigMatch.o -lpthread -ltinfo -ldl -lm

CheckApplicability:
	mkdir -p build
	clang -I$(LLVM_INCLUDE_PATH) -I$(LLVM_C_INCLUDE_PATH) -I./ -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -ffunction-sections -fdata-sections -Wcast-qual -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcovered-switch-default -Wno-uninitialized -Wno-missing-field-initializers -Wno-comment -c -MMD -MP -MF "build/Unroll.d.tmp" -MT "Unroll.o" -MT "Unroll.d" src/CheckApplicability.cpp -o build/CheckApplicability.o
	clang -O3 -Wl,-R -Wl,'$ORIGIN' -Wl,--gc-sections -rdynamic -L./ -Lm./ -shared -o build/CheckApplicability.so build/CheckApplicability.o -lpthread -ltinfo -ldl -lm

Misc:
	mkdir -p build
	clang -I$(LLVM_INCLUDE_PATH) -I$(LLVM_C_INCLUDE_PATH) -I./ -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -ffunction-sections -fdata-sections -Wcast-qual -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcovered-switch-default -Wno-uninitialized -Wno-missing-field-initializers -Wno-comment -c -MMD -MP -MF "build/Unroll.d.tmp" -MT "Unroll.o" -MT "Unroll.d" src/Misc.cpp -o build/Misc.o
	clang -O3 -Wl,-R -Wl,'$ORIGIN' -Wl,--gc-sections -rdynamic -L./ -Lm./ -shared -o build/Misc.so build/Misc.o -lpthread -ltinfo -ldl -lm


clean_extras:
	rm build/*.o 
	rm build/*.d.tmp

clean:
	rm build/*
