#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <string>  
#include <set>  

#include "Signature.h"

using namespace llvm;
using namespace std;

typedef set<Function *> FuncSet;
typedef map<Signature, FuncSet > TypeToFuncSetMap;

class SigMatch : public llvm::ModulePass {

	public:
    static char ID;
    SigMatch() : llvm::ModulePass(ID) {}
    ~SigMatch() {};
    virtual bool runOnModule(llvm::Module& module);
    void AnalyzeFuncs(Module * module);
    void AnalyzeInsts(Module * module);
    void AnalyzeGlobals(Module * module);
    void AnalyzeAliases(Module * module);
    bool InstToIgnore(Instruction * I);
    bool handleUsefulInst(Instruction * I);
    void handleConst(Constant * CC);
    void PrintFuncTargets(Module * module);
    void PrintCallStats(Module * module);
    void PrintSyscallTargets(Module * module);

    DataLayout * DL;
    TypeToFuncSetMap typesTofuncs;
};