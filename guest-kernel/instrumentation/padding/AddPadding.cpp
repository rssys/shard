
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
using namespace llvm;
using namespace std;

static cl::opt<int> padding_bytes("padding-bytes",
          cl::desc("padding bytes to add"));

namespace {

  struct Addpadding : public ModulePass {
    static char ID;
    Addpadding() : ModulePass(ID) {}
    int CountCurrentPadding(Function * F) {
      int padding_count = 0;
      for(Function::iterator b = F->begin(), e = F->end(); b != e; ++b) {
        for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
          Instruction * I = &*it;
          CallInst * CI = dyn_cast<CallInst>(I);
          if(!CI || !CI->isInlineAsm()) {
            if(!isa<ReturnInst>(I)) {
              errs() << "illegal instruction found " << *I << "\n";
            }
            continue;
          }
          padding_count++;
        }
      }
      return padding_count;      
    }

    void AddPaddingBytes(Function * F, int padding_bytes) {
      BasicBlock * entry = &*(F->begin());
      Instruction * first = &*(entry->begin());
      errs() << "Adding " << padding_bytes << " instructions\n";
      for(int i = 0; i < padding_bytes; i++) {
        Instruction * nop = first->clone();
        nop->insertBefore(first);
      }
    }

    void RemovePaddingBytes(Function * F, int padding_bytes) {
      BasicBlock * entry = &*(F->begin());
      errs() << "Removing " << padding_bytes << " instructions\n";
      for(int i = 0; i < padding_bytes; i++) {
        Instruction * first = &*(entry->begin());
        first->eraseFromParent();
      } 
    }

    bool runOnModule(Module& M) {
      errs() << "padding-bytes " << padding_bytes << "\n";
      Function * F = M.getFunction("__padding_function");
      if(!F) {
        errs() << "padding function not found\n";
        return false;
      }
      AddPaddingBytes(F, padding_bytes);

      Function * func = M.getFunction("__switch_to");
      Instruction * toRemove = NULL;
      for(Function::iterator b = func->begin(), e = func->end(); b != e; ++b) {
        for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
          Instruction * I = &*it;
          CallInst * CI = dyn_cast<CallInst>(I);
          if(!CI) continue;
          if(CI->getCalledFunction() && CI->getCalledFunction()->getName() == "set_tracking_2") {
            errs() << *CI << "\n";
            toRemove = I;
          }
        }
      }
      errs() << toRemove << "\n";
      if(toRemove) {
        errs() << toRemove << "\n";
        toRemove->eraseFromParent();
      }
      return true;
    }
  };
}

char Addpadding::ID = 0;
static RegisterPass<Addpadding> X("add-padding", "shadow stack protection", false, false);
