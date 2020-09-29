#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <string>  
#include <set>  
#include <map>  

#include "CountFunctionPointerStores.h"
#include "Signature.h"
#include "IterUtils.h"

using namespace llvm;
using namespace std;

char CountFunctionPointerStores::ID = 0;
static RegisterPass<CountFunctionPointerStores> X("CountFunctionPointerStores", "CountFunctionPointerStores Pass", false, false);

bool CountFunctionPointerStores::runOnModule(llvm::Module& module) {
    DL = new DataLayout(&module);
    Module * M = &module;
    Function * F;
    BasicBlock * BB;
    Instruction * I;
    int num = 0;
    FOR_INSTS_IN_MODULE(M, F, BB, I) {
        StoreInst * SI = dyn_cast<StoreInst>(I);
        if(!SI) continue;
        num += IS_FUNC_PTR_TYPE(SI->getValueOperand()->getType());
    }
    errs() << num << "\n";
    return false;
}