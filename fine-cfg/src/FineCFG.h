#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <string>  
#include <set>  
#include <map>  

using namespace llvm;
using namespace std;

typedef set<Function *> FuncSet;
typedef pair<StructType *, int> StructOffsetPair;
typedef pair<CallInst *, unsigned> CallArg;

enum TType {
	None,
	Variable,
	Array,
	Struct
};

struct MemTaintee {
	enum TType ttype;
	Value * val;
	StructOffsetPair structOffset;
};

typedef map<StructOffsetPair, FuncSet> SOToFuncSet;

class FineCFG : public llvm::ModulePass {

	public:
    static char ID;
	DataLayout * DL;
	SOToFuncSet structOffsetMapping;
	map<Value *, FuncSet> taintedVars;
	set<GlobalVariable *> GlobalsToAnalyze;
	map<CallInst *, FuncSet> CallsToTargets;
	map<StructType *, set<StructType * > > CastingMap;
    set<CallInst *> handledIndCallInsts;
    map<CallArg, FuncSet> TaintedCallArgs;
    map<CallArg, FuncSet> TaintedCallArgPropagated;

    FineCFG() : llvm::ModulePass(ID) {}
    ~FineCFG() {};
    virtual bool runOnModule(llvm::Module& module);
    void AnalyzeFuncs(Module * module);
    void AnalyzeGlobals();
    void InitializeGlobal(Constant * CC);
    bool HandleTaintedStore(StoreInst * SI, FuncSet funcs);
    bool HandleTaintedLoad(LoadInst * LI, MemTaintee & taintee);
    void HandleBitCast(BitCastInst * BI);
    void handleCallInst(CallInst * CI, Value * tainted, FuncSet funcs);
    void HandleConst(GlobalVariable * gv, Constant * CC);
    void PropagateBitCasts(Module * module);
    set<StructType *> getAllAliasingTypes(StructType * st);
    FuncSet getSOFunctions(StructType * st, int offset);
    void PropagateStructOffsets(Module * module);
    void PropagateTaintedVars(Module * module);
    void PropagatetaintedCallArgs(Module * module);
    void PropagateValue(Module * module, Value * val, FuncSet funcs);
	bool CheckRelevantPtr(Value * ptr, MemTaintee& taintee);
	void PrintFuncTargets(Module * module);
};