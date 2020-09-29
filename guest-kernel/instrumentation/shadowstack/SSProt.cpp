
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
#include "IterUtils.h"
using namespace llvm;
using namespace std;
/*
string BlackList[] = {
  "__init_waitqueue_head",
  "__wake_up",
  "kvfree",
  "kvfree",
  "up_read",
  "synchronize_sched",
  "__wake_up_locked",
  "blk_start_plug",
  "blk_start_plug",
  "blk_finish_plug",
  "lru_add_drain",
  "up_write",
  "sort",
  "percpu_ref_exit",
  "init_wait_entry",
  "__mutex_init",
  "unhash_mnt",
  "device_shutdown",
  "syscore_shutdown",
  "machine_restart",
  "locks_free_lock",
  "lockref_get",
  "group_pin_kill",
  "hrtimer_init",
  "ipc_set_key_private",
  "ipc_rmid",
  "free_msg",
  "put_unused_fd",
  "fd_install",
  "putname",
  "putname",
  "putname",
  "lru_add_drain_all",
  "iov_iter_advance",
  "si_meminfo",
  "si_swapinfo",
  "get_avenrun",
  "alarm_init",
  "jiffies_to_timespec64",
  "__getparam_dl",
  "inotify_remove_from_idr",
  "set_current_blocked",
  "ep_destroy_wakeup_source",
  "uts_proc_notify",
  "disable_swap_slots_cache_lock",
  "_enable_swap_info",
  "reenable_swap_slots_cache_unlock",
  "exit_swap_address_space",
  "exit_swap_address_space",
  "chroot_fs_refs",
  "kernel_to_ipc64_perm",
  "mutex_unlock",
  "mutex_lock",
  "down_read",
  "down_write",
};

#define num_black_listed sizeof(BlackList)/sizeof(string)
bool IsBlackListed(string funcName) {
  for(unsigned i = 0; i < num_black_listed; i++) {
    if(BlackList[i] == funcName) {
      return true;
    }
  }
  return false;
}*/

namespace {

  struct SSProt : public ModulePass {
    static char ID;
    SSProt() : ModulePass(ID) {}
    CallInst * CreateCall(Module * M, Function * F, vector<Value *> args) {
      CallInst * CI = CallInst::Create (F->getFunctionType(), F, ArrayRef<Value *>(args));
      return CI;
    }
    CallInst * CreateCall(Module * M, Function * F) {
      vector<Value *> args;      
      return CreateCall(M, F, args);
    }
    void call_to_ss(Module * M, Function * ss_func, Instruction * ib) {
      Function * ret_addr = Intrinsic::getDeclaration(M, Intrinsic::addressofreturnaddress);
      vector<Value *> args_r;
      CallInst * r_en = CreateCall(M, ret_addr, args_r);
      r_en->insertBefore(ib);
      BitCastInst * BI = new BitCastInst(r_en, Type::getInt64PtrTy(M->getContext()), "", ib);
      // LoadInst * LI = new LoadInst(BI, "", ib);
      vector<Value *> args;
      args.push_back(BI);
      CallInst * ci_ss = CreateCall(M, ss_func, args);
      ci_ss->insertBefore(ib);
      ci_ss->setTailCall(false);
    }

    void InstrumentSetTracking(Module &M) {
      Function * set_tracking = M.getFunction("set_tracking");
      if(!set_tracking) return;
      Function * save_old_ss = M.getFunction("save_old_ss");
      Function * add_new_ss = M.getFunction("add_new_ss");

      Instruction * first = &*(&*set_tracking->begin())->begin();
      CallInst * ci_sos = CreateCall(&M, save_old_ss);
      ci_sos->insertBefore(first);
      for(Function::iterator b = set_tracking->begin(), e = set_tracking->end(); b != e; ++b) {
        for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
          CallInst * ci_ud2 = dyn_cast<CallInst>(&*it);
          if(!ci_ud2 || !ci_ud2->getFunction() || ci_ud2->getCalledFunction()->getName() != "ud2_call")
            continue;
          vector<Value *> args;
          args.push_back(ci_ud2->getArgOperand(0));
          args.push_back(ci_ud2->getArgOperand(1));
          CallInst * ci_ans = CallInst::Create(add_new_ss->getFunctionType(), add_new_ss, ArrayRef<Value *>(args));
          ci_ans->insertBefore(ci_ud2);
        }
      }
    }
    // void addAllDirectEdges(Function * F, set<Function *>& workset) {
    //   for(Function::iterator b = F->begin(), e = F->end(); b != e; ++b) {
    //     for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
    //       CallInst * CI = dyn_cast<CallInst>(&*it);
    //       if(!CI) continue;
    //       Function * callee = dyn_cast<Function>(CI->getCalledValue());
    //       if(!callee) continue;
    //       if(IsBlackListed(callee)) {
    //         continue;
    //       }
    //       if(callee->getName() == "schedule")
    //         errs() << "adding schedule from " << F->getName() << "\n";
    //       workset.insert(callee);
    //     }
    //   }
    // }
    bool runOnModule(Module& M) {
      Function * ss_entry = M.getFunction("ss_entry");
      Function * ss_exit = M.getFunction("ss_exit");
      int num_instrumented = 0;
      for(Module::iterator mit = M.getFunctionList().begin(); mit != M.getFunctionList().end(); ++mit) {
        Function * func = &*mit;
        if(func->isDeclaration()) continue;
        if(IsBlackListed(func)) continue;
        string name = func->getName();
        if(name == "__iowrite64_copy") continue;
        Instruction * orig_entry = &*(func->begin())->begin();
        vector<Instruction *> returns;
        for(Function::iterator b = func->begin(), e = func->end(); b != e; ++b) {
          for(BasicBlock::iterator it = (&*b)->begin(), it2 = (&*b)->end(); it != it2; it++) {
            Instruction * I = &*it;
            if(!isa<ReturnInst>(I)) continue;
            returns.push_back(I);
          }
        }
        if(returns.size() != 1) {
          continue;
        }
        num_instrumented++;
        call_to_ss(&M, ss_entry, orig_entry);
        // errs() << *ci_entry << "\n";
        call_to_ss(&M, ss_exit, returns[0]);
        // errs() << *ci_exit << "\n";      
        errs() << func->getName() << "\n";
      }
      InstrumentSetTracking(M);
      errs() << "num ss protections " << num_instrumented << "\n";
      return true;
    }
  };
}

char SSProt::ID = 0;
static RegisterPass<SSProt> X("ss-protection", "shadow stack protection", false, false);