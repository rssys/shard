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
#include <map>
#include <iostream>
#include <fstream>
#include <time.h>
#include <fstream>

#include "IterUtils.h"
#include "blacklist.h"
#include "cfi_lib.h"

using namespace llvm;
using namespace std;


#define IS_INDIRECT_CALL_SITE(ci) (!ci->isInlineAsm() && !isa<Function>(ci->getCalledValue()->stripPointerCasts()) && !isa<GlobalAlias>(CI->getCalledValue()->stripPointerCasts()))

namespace {
  struct CFIInst : public ModulePass {
    static char ID;
    CFIInst() : ModulePass(ID) {}
    map<int, set<string> > CITargets;
    void init_cfg() {
      std::ifstream infile("callsites_info");
      string line;
      int curr_ci;
      while(getline(infile, line)) {
      	if (line.size() > 9 and line.substr(0, 9) == "indirect:") {
      		curr_ci = stoi(line.substr(9, line.size()));
          if(CITargets.find(curr_ci) != CITargets.end()) {
            errs() << "curr_ci is " << curr_ci << " already present\n";
        		assert(0);
          }
      	} else {
      		CITargets[curr_ci].insert(line);
      	}
      }
    }

    CallInst * CreateCall(Function * F, vector<Value *> args, Instruction * before) {
      CallInst * CI = CallInst::Create(F->getFunctionType(), F, ArrayRef<Value *>(args));
      CI->insertBefore(before);
      return CI;
    }

    void InstrumentCtxSwitch(Module * M) {
      Function * switch_to = M->getFunction("__switch_to");
      Function * initialize = M->getFunction("initialize");
      Instruction * first = &*(&*switch_to->begin())->begin();
      vector<Value *> args;
      CreateCall(initialize, args, first);
    }

    void SetCallSiteTargets(Module * M, vector<Constant *> callsiteTargets) {
      StructType * ty = M->getTypeByName("struct.CallSite_to_Func"); 
      for(unsigned i = 0; i < (num_call_site_targets - callsiteTargets.size()); i++) {
        vector<Constant *> structConsts;
        ConstantInt * zero = ConstantInt::get(Type::getInt32Ty(M->getContext()), 0);
        structConsts.push_back(zero);
        structConsts.push_back(ConstantPointerNull::get(dyn_cast<PointerType>(ty->getContainedType(1))));          
        Constant * CS = ConstantStruct::get(ty, ArrayRef<Constant *>(structConsts));
        callsiteTargets.push_back(CS);
      }
      GlobalVariable * num_callsite_targets = M->getNamedGlobal("num_callsite_targets");
      GlobalVariable * callsite_targets = M->getNamedGlobal("callsite_targets");
      ConstantInt * num = ConstantInt::get(Type::getInt32Ty(M->getContext()), callsiteTargets.size());
      num_callsite_targets->setInitializer(num);
      ArrayType * Aty = ArrayType::get(ty, num_call_site_targets);
      callsite_targets->setInitializer(ConstantArray::get(Aty, ArrayRef<Constant *>(callsiteTargets)));
    }

    void InstrumentCallSites(Module * M) {
      // Function * add_to_ht = M->getFunction("add_to_ht");
      Function * check_ci = M->getFunction("check_ci");      
      // Function * initialize_cs = M->getFunction("initialize_cs");
      // initialize_cs->setSection(".init.text");
      // Instruction * first = &*(&*initialize_cs->begin())->begin();
      Function * F;
      BasicBlock * BB;
      Instruction * I;
      int callsite_idx = 0;
      vector<Constant *> callsiteTargets;
      StructType * ty = M->getTypeByName("struct.CallSite_to_Func"); 

      FOR_FUNCS_IN_MODULE(M, F) {
        FOR_INSTS_IN_FUNC(F, BB, I) {
          CallInst * CI = dyn_cast<CallInst>(I);
          if(!CI) continue;
          if(CI->isInlineAsm()) continue;
          if(Function * target = dyn_cast<Function>(CI->getCalledValue()->stripPointerCasts())) continue;
          callsite_idx = stoi(cast<MDString>(I->getMetadata("callsite.id")->getOperand(0))->getString());
          ConstantInt * Id = ConstantInt::get(Type::getInt32Ty(M->getContext()), callsite_idx);
          if(F->getName() == "do_syscall_64") {
            errs() << "Call site in do_syscall_64 : id is " << callsite_idx << " number of targets is " << CITargets[callsite_idx].size() << "\n"; 
          }
          if(IsBlackListed(F)) {
          	continue;
          }

         for(auto str : CITargets[callsite_idx]) {
            Function * target = M->getFunction(str);
            if(!target) {
              errs() << str << " not found in Module\n";
              continue;
            }
            vector<Constant *> structConsts;
            structConsts.push_back(Id);
            structConsts.push_back(ConstantExpr::getBitCast(target, ty->getContainedType(1)));
            Constant * CS = ConstantStruct::get(ty, ArrayRef<Constant *>(structConsts));
            callsiteTargets.push_back(CS);
            /*
            vector<Value *> args;
            PtrToIntInst * PII = new PtrToIntInst(target, Type::getInt64Ty(M->getContext()), "", first);
            args.push_back(Id);
            args.push_back(PII);
            CreateCall(add_to_ht, args, first);
            */
          }
          vector<Value *> args;
          PtrToIntInst * PII = new PtrToIntInst(CI->getCalledValue(), Type::getInt64Ty(M->getContext()), "", CI);
          args.push_back(Id);
          args.push_back(PII);
          CreateCall(check_ci, args, CI);
        }
      }
      SetCallSiteTargets(M, callsiteTargets);
      errs() << "callsite_idx " << callsite_idx << "\n";
    }

    bool runOnModule(Module& M) {
      init_cfg();
      InstrumentCtxSwitch(&M);
      InstrumentCallSites(&M);
      return true;
    }
  };
}

char CFIInst::ID = 0;
static RegisterPass<CFIInst> X("CFIInst", "CFI Instrumentation", false, false);