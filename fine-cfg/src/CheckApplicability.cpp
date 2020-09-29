#include "CheckApplicability.h"
#include "IterUtils.h"
#include "Utils.h"

char CheckApplicability::ID = 0;
static RegisterPass<CheckApplicability> X("CheckApplicability", "CheckApplicability Pass", false, false);

void CheckApplicability::runOnInst(Module * module) {
	int num_ind_calls = 0;
	int num_struct_loads = 0;
	int num_select_insts = 0;
	int num_call_args = 0;
	Function * F; 
	BasicBlock * BB;
	Instruction * I;
	FOR_INSTS_IN_MODULE(module, F, BB, I) {
		if(CallInst * ci = dyn_cast<CallInst>(I)) {
			if(ci->isInlineAsm())
				continue;
			Value * ptr = ci->getCalledValue()->stripPointerCasts();
			if(isa<Function>(ptr)) continue;
			if(isa<GlobalAlias>(ptr)) continue;
			num_ind_calls++;
			if(CheckStructLoad(ci, DL)) num_struct_loads++;
		}
	}
	errs() << num_struct_loads << ", " << num_select_insts << ", " << num_call_args << " / " << num_ind_calls << "\n";
}

bool CheckApplicability::runOnModule(llvm::Module& module) {
	DL = new DataLayout(&module);
	runOnInst(&module);
	return false;
}