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


namespace {
  struct SCInst : public ModulePass {
    static char ID;
    SCInst() : ModulePass(ID) {}

    Value * getArg(Function * func, int index){
      int i = 0;
      for(auto arg = func->arg_begin(), argEnd = func->arg_end(); arg != argEnd; arg++){
        if(i == index) 
          return &*arg;
        i++;
      }
      return NULL;  
    }
    
    CallInst * CreateCall(Function * F, vector<Value *> args) {
      CallInst * CI = CallInst::Create(F->getFunctionType(), F, ArrayRef<Value *>(args));
      return CI;
    }

    CallInst * CreateCall(Function * F) {
      vector<Value *> args;
      return CreateCall(F, args);
    }

    void InstrumentSetTracking(Module &M) {
      Function * switch_to = M.getFunction("__switch_to");
      Function * set_tracking = M.getFunction("set_tracking");
      errs() << "set_tracking - " << set_tracking << "\n";
      for(Function::iterator b = switch_to->begin(), e = switch_to->end(); b != e; ++b) {
        for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
          Instruction * I = dyn_cast<Instruction>(&*it);
          CallInst * CI = dyn_cast<CallInst>(I);
          if(!CI) continue;
          if(CI->getCalledFunction() && CI->getCalledFunction()->getName() == "set_tracking_2") {
            errs() << *CI << "\n";
            CI->setCalledFunction(set_tracking);
          }
        }
      }
    }
    
    bool runOnModule(Module& M) {
      errs() << "entered SCInst\n";
      Function * sc_entry = M.getFunction("sc_entry");
      Function * sc_exit = M.getFunction("sc_exit");
      Function * func = M.getFunction("do_syscall_64");

      BasicBlock * BBtoInsert = NULL;
      errs() << sc_entry << " " << sc_exit << " " << func << "\n";
      Value * sc_id;
      for(Function::iterator b = func->begin(), e = func->end(); b != e; ++b) {
        for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
          User * user = dyn_cast<User>(&*it);
          for(unsigned i = 0; i < user->getNumOperands(); i++) {
            ConstantExpr * CE = dyn_cast<ConstantExpr>(user->getOperand(i));
            if(!CE) continue;
            Instruction * I = CE->getAsInstruction();
            if(!I) continue;
            BitCastInst * BI = dyn_cast<BitCastInst>(I);
            if(!BI) {
              I->dropAllReferences();
              continue;
            }
            User * user = dyn_cast<User>(BI);
            for(unsigned i = 0; i < user->getNumOperands(); i++) {
              GlobalVariable * GV = dyn_cast<GlobalVariable>(user->getOperand(i));
              if(!GV) continue;
              if(GV->getName() != "sys_call_table") continue;
              BBtoInsert = &*b;
              GetElementPtrInst * GI = dyn_cast<GetElementPtrInst>(&*it);
              sc_id = dyn_cast<Value>(&*(GI->idx_begin() + 1));
              vector<Value *> args;
              args.push_back(sc_id);
              CallInst * CI = CallInst::Create (sc_entry->getFunctionType(), sc_entry, ArrayRef<Value *>(args));
              errs() << *GI << " ----------------------> sc_entry\n";
              CI->insertBefore(GI);
            }
            I->dropAllReferences();
          }
        }
      }
      Instruction * I = BBtoInsert->getTerminator();
      vector<Value *> args;
      args.push_back(sc_id);
      errs() << *I << " ----------------> sc_exit\n";
      CallInst * CI = CallInst::Create(sc_exit->getFunctionType(), sc_exit, ArrayRef<Value *>(args));
      CI->insertBefore(I);
      InstrumentSetTracking(M);

      Function * log_irq_enter = M.getFunction("log_irq_enter");
      Function * log_irq_exit = M.getFunction("log_irq_exit");
      Function * irq_enter = M.getFunction("irq_enter");
      Function * irq_exit = M.getFunction("irq_exit");

      errs() << log_irq_enter << " " << log_irq_exit << " " << irq_enter << " " << irq_exit << "\n";

      Instruction * entry = &*(irq_enter->begin())->begin();
      CreateCall(log_irq_enter)->insertBefore(entry);
      vector<Instruction *> returns;
      for(Function::iterator b = irq_exit->begin(), e = irq_exit->end(); b != e; ++b) {
        for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
          Instruction * I = &*it;
          if(!isa<ReturnInst>(I)) continue;
          returns.push_back(I);
        }
      }
      for(unsigned i = 0; i < returns.size(); i++) {
        CreateCall(log_irq_exit)->insertBefore(returns[i]);
      }
      return true;
    }
  };
}

char SCInst::ID = 0;
static RegisterPass<SCInst> X("sc-instrumentation", "sc-instrumentation", false, false);
