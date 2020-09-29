#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <string>  

using namespace llvm;
using namespace std;

class CheckApplicability : public llvm::ModulePass {

	public:
    static char ID;
	DataLayout * DL;
    CheckApplicability() : llvm::ModulePass(ID) {}
    ~CheckApplicability() {};
    virtual bool runOnModule(llvm::Module& module);
    void runOnInst(Module * module);
};