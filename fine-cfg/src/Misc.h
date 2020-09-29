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

class Misc : public llvm::ModulePass {
	public:
    static char ID;
	DataLayout * DL;
    Misc() : llvm::ModulePass(ID) {}
    ~Misc() {};
    virtual bool runOnModule(llvm::Module& module);
};