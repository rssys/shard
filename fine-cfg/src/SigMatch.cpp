#include "SigMatch.h"
#include "IterUtils.h"
#include "StdUtils.h"
#include "Utils.h"


char SigMatch::ID = 0;
static RegisterPass<SigMatch> X("SigMatch", "SigMatch Pass", false, false);

bool SigMatch::runOnModule(Module& M) {
	DL = new DataLayout(&M);
	AnalyzeFuncs(&M);
	AnalyzeAliases(&M);
	AnalyzeGlobals(&M);
	AnalyzeInsts(&M);
	PrintFuncTargets(&M);
	// PrintCallStats(&M);
	// PrintSyscallTargets(&M);
	return true;
}

bool HasAddrTakenInInst(Function * F) {
	vector<Value *> worklist;
	worklist.push_back(F);
    while(worklist.size()) {
		Value * worker = dyn_cast<Value>(worklist[0]);
		worklist.erase(worklist.begin());
		for(Use &U : worker->uses()) {
			User * user = U.getUser();
			if(Instruction * I = dyn_cast<Instruction>(user)) {
				if(CallInst * ci = dyn_cast<CallInst>(I)) {
					Function * callee = ci->getCalledFunction();
					if(callee && callee == F)
						continue;
				}
				return true;
			}
			if(isa<GlobalObject>(user) || isa<Constant>(user)) {
				worklist.push_back(user);
			} else {
				errs() << "unexpected use of function " << *user << "\n";
			}
		}
    }
    return false;
}

void SigMatch::AnalyzeFuncs(Module * module) {
	int num = 0;
	Function * F;
	FOR_FUNCS_IN_MODULE(module, F) {
		if(!F->hasAddressTaken())
			continue;
		if(!HasAddrTakenInInst(F))
			continue;

		num += 1;
		Signature sign = getSignature(F);
		typesTofuncs[sign].insert(F);
	}
	// errs() << "number of functions considered is " << num << "\n";
}

void SigMatch::AnalyzeAliases(Module * module) {
	for(auto& alias : module->aliases()) {
		GlobalAlias * ga = &alias;
		handleConst(ga->getAliasee());
	}
}

void SigMatch::AnalyzeGlobals(Module * module) {
	for(auto& global : module->globals()) {
		GlobalVariable *  gv = &global;
		if(gv->getName() == "llvm.used")
			continue;
		if(gv->hasInitializer()) 
			handleConst(gv->getInitializer());
	}
}

void SigMatch::handleConst(Constant * CC) {
	vector<Value *> worklist;
	vector<Value *> processed;
	worklist.push_back(CC);
    while(worklist.size()) {
		Value * worker = dyn_cast<Constant>(worklist[0]);
		worklist.erase(worklist.begin());
		if(!worker)
			continue;
		if(isa<GlobalObject>(worker))
			continue;
		if(ConstantExpr * CE = dyn_cast<ConstantExpr>(worker)) {
			Instruction * tmp = CE->getAsInstruction();
			handleUsefulInst(tmp);
			if(!tmp->getParent()) tmp->dropAllReferences();
		}
		User * user = dyn_cast<User>(worker);
    	if(!user) continue;
    	for(unsigned i = 0; i < user->getNumOperands(); i++) {
    		worklist.push_back(user->getOperand(i));
    	}
    	processed.push_back(worker);
    }
}

void SigMatch::AnalyzeInsts(Module * module) {
	Function * F;
	BasicBlock * BB;
	Instruction * I;
	FOR_INSTS_IN_MODULE(module, F, BB, I) {
		if(handleUsefulInst(I)) {}
		else {
			if(InstToIgnore(I) || isa<CallInst>(I)) continue;
			// errs() << "Found unexpected Instruction " << *I << "\n";
			if(!isa<IntToPtrInst>(I) && !isa<PtrToIntInst>(I) && !isa<AddrSpaceCastInst>(I))
				assert(0);
		}
	}
}


bool SigMatch::InstToIgnore(Instruction * I) {
	if(isa<AllocaInst>(I) || isa<StoreInst>(I) || isa<LoadInst>(I) || isa<GetElementPtrInst>(I)
	|| isa<PHINode>(I) || isa<TerminatorInst>(I) || isa<CmpInst>(I)
	|| isa<BinaryOperator>(I) || isa<SelectInst>(I))
		return true;

	if(isa<CastInst>(I) && !isa<BitCastInst>(I) && !isa<IntToPtrInst>(I) && !isa<PtrToIntInst>(I)
	&& !isa<AddrSpaceCastInst>(I))
		return true;

	bool isUseful = false;
	if(User * user = dyn_cast<User>(I)) {
    	for(unsigned i = 0; i < user->getNumOperands(); i++) {
    		Value * val = user->getOperand(i);
    		if(IS_FUNC_PTR_TYPE(val->getType()))
    			isUseful = true;
    	}
    }	
	return !isUseful;
}

bool SigMatch::handleUsefulInst(Instruction * I) {	
	if(BitCastInst * BI = dyn_cast<BitCastInst>(I)) {
		Type * ST = BI->getSrcTy();
		Type * DT = BI->getDestTy();
		if(IS_FUNC_PTR_TYPE(ST)) {
			if(!IS_FUNC_PTR_TYPE(DT)) {
				// errs() << *ST->getContainedType(0) << "\n";
				// errs() << *DT->getContainedType(0) << "\n";
				// errs() << *I << " Func to something else\n";
			} else {
				Signature destSign = getSignature(DT); 
				FuncSet SFS = typesTofuncs[getSignature(ST)];
				for(auto func : SFS)
					typesTofuncs[destSign].insert(func);
			}
		}
		return true;
	}

	if(User * user = dyn_cast<User>(I)) {
    	for(unsigned i = 0; i < user->getNumOperands(); i++) {
    		if(ConstantExpr * CE = dyn_cast<ConstantExpr>(user->getOperand(i))) {
    			Instruction * tmp = CE->getAsInstruction();
    			handleUsefulInst(tmp);
    			if(!tmp->getParent()) tmp->dropAllReferences();
    		}
    	}
    }
	return false;
}


void checkCallSiteId(Instruction * I, int id) {
	assert(cast<MDString>(I->getMetadata("callsite.id")->getOperand(0))->getString() == to_string(id));
}

void SigMatch::PrintFuncTargets(Module * module) {
	Function * F;
	BasicBlock * BB;
	Instruction * I;
	int num_indirect_call_sites = 0;
	int num_funcs = 0;
	FOR_FUNCS_IN_MODULE(module, F) {
		errs() << F->getName() << "{\n";
		if(F->isDeclaration())
		    errs() << "__External_Function__\n";
		num_funcs++;
		FOR_INSTS_IN_FUNC(F, BB, I) {
		    if(CallInst * ci = dyn_cast<CallInst>(I)) {
		        if(ci->isInlineAsm())
		            continue;
		        Value * ptr = ci->getCalledValue()->stripPointerCasts();
		        if(Function * target = dyn_cast<Function>(ptr)) {
		            errs() << "callsite:direct 1\n";
	            	if(F->getName() == "SyS_clone" && (target->getName() == "prepare_kernel_cred" || target->getName() == "commit_creds")) {
	            		errs() << "SyS_clone\n";
	            		continue;            
	            	}
		            errs() << target->getName() << "\n";
		        } else {
			        if(F->getName() == "___bpf_prog_run") {
		        		errs() << "callsite:" << "indirect" << " " << -1 << "\n";
		        		checkCallSiteId(ci, num_indirect_call_sites);
			            num_indirect_call_sites++;
			            continue;
				    }
		        	Signature sign = getSignature(ptr->getType());
		            FuncSet targets = typesTofuncs[sign];
	        		errs() << "callsite:" << "indirect" << " " << targets.size() << "\n";
	        		checkCallSiteId(ci, num_indirect_call_sites);
		            num_indirect_call_sites++;
					for(auto func : targets)
						errs() << func->getName() << "\n";
		        }
		    }
		}
		errs() << "}\n";
	}
	errs() << num_indirect_call_sites << " " << num_funcs << "\n";
}

// void SigMatch::PrintCallStats(Module * module) {
// 	int num_calls = 0;
// 	int num_icalls = 0;
// 	map<Signature, int > SigToNumberCSMap;
// 	INITIALIZE_INST_ITER(m_it, f_it, b_it, I);
// 	do {
// 		if((&*m_it)->getName() == "___bpf_prog_run")
// 			continue;
// 	    if(CallInst * ci = dyn_cast<CallInst>(I)) {
// 	        if(ci->isInlineAsm())
// 	            continue;
// 	        num_calls++;
// 	        Value * ptr = ci->getCalledValue()->stripPointerCasts();
// 	        if(Function * target = dyn_cast<Function>(ptr)) {} 
// 	        else {
// 	        	Signature sign = getSignature(ptr->getType());
// 	        	if(!findInMap(SigToNumberCSMap, sign))
// 	        		SigToNumberCSMap[sign] = 0;
// 	        	SigToNumberCSMap[sign]++;
// 	        	num_icalls++;
// 	    	}
// 	    }		
// 	} while((I = getNextInst(module, &m_it, &f_it, &b_it)));

// 	errs() << num_calls << " " << num_icalls << "\n";

// 	for(auto pair : typesTofuncs) {
// 		errs() << getSignStr(pair.first) << " " << pair.second.size() << "\n";
// 	}

// 	errs() << "***********************************************\n";

// 	for(auto pair : SigToNumberCSMap) {
// 		errs() << getSignStr(pair.first) << " " << pair.second << "\n";
// 	}
// }

// void SigMatch::PrintSyscallTargets(Module * module) {
// 	Function * F = module->getFunction("do_syscall_64");
// 	INITIALIZE_FUNC_INST_ITER(F, f_it, b_it, I);
// 	do {
// 	    if(CallInst * ci = dyn_cast<CallInst>(I)) {
// 	        if(ci->isInlineAsm())
// 	            continue;
// 	        errs() << *ci << " ";
// 	        Value * ptr = ci->getCalledValue()->stripPointerCasts();
// 	        if(Function * target = dyn_cast<Function>(ptr)) {
// 	            errs() << 1 << "\n";
// 	            errs() << target->getName() << "\n";
// 	        } else {
// 	        	Signature sign = getSignature(ptr->getType());
// 	            FuncSet targets = typesTofuncs[sign];
// 	            errs() << targets.size() << "\n";
// 				for(auto func : targets)
// 					errs() << func->getName() << "\n";
// 	        }
// 	    }		
// 	} while((I = getNextInst(F, &f_it, &b_it)));
// }