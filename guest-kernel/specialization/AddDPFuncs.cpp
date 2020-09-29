
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/CommandLine.h"
#include <llvm/Analysis/LoopInfo.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <string> 
#include <unistd.h> 
#include <set>
#include <iostream>
#include <fstream>
#include "blacklist.h"

using namespace llvm;
using namespace std;

/*
#define num_white_listed 14
string WhiteList[num_white_listed] = {
  "acpi_pm_read_slow",
  "jiffies_read",
  "read_hpet",
  "si_meminfo",
  "si_swapinfo",
  "SyS_sysinfo",
  "nr_blockdev_pages",
  "ns_to_timespec",
  "ktime_get_with_offset",
  "dummy_clock_read",
  "get_avenrun",
  "_copy_to_user",
  "acpi_pm_read",
  "read_tsc"
};

bool IsWhiteListed(string funcName) {
  for(unsigned i = 0; i < num_white_listed; i++) {
    if(WhiteList[i] == funcName) {
      return true;
    }
  }
  return false;
}*/

namespace {

  struct SSProt : public ModulePass {
    static char ID;
    SSProt() : ModulePass(ID) {}
    GlobalVariable * materializeStringLiteral(Module& M, const char* data) {
      Constant * ary = ConstantDataArray::getString(M.getContext(), StringRef(data), false);
      GlobalVariable * gv = new GlobalVariable(M, ary->getType(), true, GlobalValue::PrivateLinkage, ary);
      return gv;
    }

    Instruction * AddGlobalGEP(Module& M, string str) {
      GlobalVariable * GV = materializeStringLiteral(M, str.c_str());
      Type * GVT = GV->getType()->getElementType();
      vector<Value *> indices;
      indices.push_back(ConstantInt::get(IntegerType::get(M.getContext(), 32), 0));
      indices.push_back(ConstantInt::get(IntegerType::get(M.getContext(), 32), 0));
      GetElementPtrInst * GI = GetElementPtrInst::Create(GVT, GV, ArrayRef<Value *>(indices));
      return GI;
    }

    bool runOnModule(Module& M) {
      // errs() << sizeof(BlackList)/sizeof(string) << "\n";
      Function * log_fn = M.getFunction("log_fn");
      int num_instrumented = 0;
      for(Module::iterator mit = M.getFunctionList().begin(); mit != M.getFunctionList().end(); ++mit) {
        Function * func = &*mit;
        if(func->isDeclaration()) continue;
        if(IsBlackListed(func)) continue;
        // errs() << func->getName() << "\n";
        Instruction * first = &*(&*func->begin())->begin();

        ConstantInt * Id = ConstantInt::get(IntegerType::get(M.getContext(), 32), num_instrumented);
        ConstantInt * size = ConstantInt::get(IntegerType::get(M.getContext(), 32), func->getName().size());
        Instruction * GI = AddGlobalGEP(M, func->getName());
        GI->insertBefore(first);
        vector<Value *> args;
        args.push_back(Id);
        args.push_back(GI);
        args.push_back(size);
        CallInst * CI = CallInst::Create(log_fn->getFunctionType(), log_fn, ArrayRef<Value *>(args));
        CI->insertBefore(first);
        num_instrumented++;
      }
      errs() << num_instrumented << " functions instrumented\n";
      return true;
    }
  };
}

char SSProt::ID = 0;
static RegisterPass<SSProt> X("dp-funcs", "profiling functions", false, false);
