#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Metadata.h"
#include <string>  
#include <set>  
#include <map>  

using namespace llvm;
using namespace std;

class CountFunctionPointerStores : public llvm::ModulePass {
	public:
    static char ID;
	DataLayout * DL;
    CountFunctionPointerStores() : llvm::ModulePass(ID) {}
    ~CountFunctionPointerStores() {};
    virtual bool runOnModule(llvm::Module& module);
};