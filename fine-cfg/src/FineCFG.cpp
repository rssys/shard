#include "FineCFG.h"
#include "IterUtils.h"
#include "StdUtils.h"
#include "Utils.h"
#include "Debug.h"
#include "TypeAliasing.cpp"

char FineCFG::ID = 0;
static RegisterPass<FineCFG> X("FineCFG", "FineCFG Pass", false, false);

set<StoreInst *> taintedStores;

bool FineCFG::runOnModule(llvm::Module& module) {
	initDebug();
	DL = new DataLayout(&module);
	TypeAliasingInfo(&module);
	PropagateBitCasts(&module);
	AnalyzeFuncs(&module);
	PropagateTaintedVars(&module);
	PrintFuncTargets(&module);
	errs() << "number of tainted stores is " << taintedStores.size() << "\n";
	return false;
}

void FineCFG::AnalyzeFuncs(Module * module) {
	Function * F;
	int num_funcs = 0;
	FOR_FUNCS_IN_MODULE(module, F) {
		num_funcs++;
		if(!F->hasAddressTaken())
			continue;
		taintedVars[F].insert(F);		
	}
	errs() << num_funcs << "\n";
}

void FineCFG::HandleConst(GlobalVariable * gv, Constant * CC) {
	CC = CC->stripPointerCasts();
	if(ConstantStruct * CS = dyn_cast<ConstantStruct>(CC)) {
		for(unsigned i = 0; i < CC->getNumOperands(); i++) {
			Value * val = CC->getAggregateElement(i)->stripPointerCasts();
			if(!IS_FUNC_PTR_TYPE(val->getType())) {
				HandleConst(gv, dyn_cast<Constant>(val));				
				continue;
			}
			if(isa<ConstantPointerNull>(val))
				continue;
			StructType * ty = dyn_cast<StructType>(CC->getType());
			int offset = getStructFieldOffset(ty, i, DL);
			assert(isa<Function>(val));
			debug() << "handleConst " << ty->getName() << " " << offset << "\n";
			debug() << val->getName() << "\n";
			structOffsetMapping[make_pair(ty, offset)].insert(dyn_cast<Function>(val));
		}
	} else if(ConstantArray * CA = dyn_cast<ConstantArray>(CC)) {
		for(unsigned i = 0; i < CC->getNumOperands(); i++) {
			Constant * CGI = CC->getAggregateElement(i);
			HandleConst(gv, CGI);
		}
	} else if(isa<GlobalValue>(CC)) {
		// GlobalsToAnalyze.insert(CC); // check if already analyzed
	} else if(isa<ConstantData>(CC)) {
	} else if(isa<ConstantExpr>(CC)) {
	} else if(isa<BlockAddress>(CC)) {
	} else {
		errs() << *gv << "\n";
		errs() << *CC << " not handled\n";
		errs() << CC->getType()->isPointerTy() << "\n";
		ERROR_EXIT("Unhandled Case When Handling Global Initializations\n");
	}
}

void FineCFG::AnalyzeGlobals() {
	for(auto gv : GlobalsToAnalyze) {
		if(!gv->hasInitializer()) {
			ERROR_EXIT("Unexpected Scenario : Tainted Global does not have Initializer\n");
     	}
     	debug() << gv->getName() << "\n";
     	HandleConst(gv, gv->getInitializer());
	}
}

set<StructType *> FineCFG::getAllAliasingTypes(StructType * st) {
	set<StructType *> seen;
	vector<StructType *> worklist;
	StructType * worker;
	QUEUE_ITERATE(worklist, st, worker) {
		if(findInSet(seen, worker))
			continue;
		seen.insert(worker);
		for(auto edge : CastingMap[worker]) {
			worklist.push_back(edge);
		}
	}
	return seen;
}

FuncSet FineCFG::getSOFunctions(StructType * ST, int offset) {
	FuncSet fset;
	StructSet seen;
	StructSet SameName = StructsWithSameName(ST);
	vector<StructType *> worklist;
	StructType * worker;
	for(auto worker : SameName) {
		worklist.push_back(worker);
	}
	QUEUE_ITERATE(worklist, ST, worker) {
		if(findInSet(seen, worker))
			continue;
		seen.insert(worker);
		addSetToSet(fset, structOffsetMapping[make_pair(worker, offset)]);
		// if(!SameStruct(st, worker)) continue;
		// if(!hasFuncPointerAtOffset(worker, offset, DL)) continue;
		if(!findInSet(SameName, worker) && hasName(worker)) continue;
		for(auto edge : CastingMap[worker]) {
			worklist.push_back(edge);
		}
	}
	return fset;
}

void FineCFG::PropagatetaintedCallArgs(Module * module) {
	for(auto ent : TaintedCallArgs) {
		CallInst * CI = ent.first.first;
		unsigned idx = ent.first.second;
		FuncSet funcs = ent.second;
		Value * ptr = CI->getCalledValue()->stripPointerCasts();
		if(!IS_INDIRECT_CALL_SITE(CI)) {
			Function * F = dyn_cast<Function>(ptr);
			if(F->isVarArg() || F->isDeclaration()) continue;
			PropagateValue(module, getArg(F, idx), funcs);
		} else {
			FuncSet targets = CallsToTargets[CI];
			for(auto target : targets) {
				if(target->isVarArg() || target->isDeclaration()) continue;
				if(findInSet(TaintedCallArgPropagated[ent.first], target))
					continue;
				TaintedCallArgPropagated[ent.first].insert(target);
				PropagateValue(module, getArg(target, idx), funcs);
			}
		}
	}
}

void FineCFG::PropagateStructOffsets(Module * module) {
	Function * F;
	BasicBlock * BB;
	Instruction * I;

	FOR_INSTS_IN_MODULE(module, F, BB, I) {
		LoadInst * LI = dyn_cast<LoadInst>(I);
		MemTaintee taintee;
		if(!LI || !IS_FUNC_PTR_TYPE(LI->getType()))
			continue;
		if(!HandleTaintedLoad(LI, taintee)) {
			continue;
			// ERROR_EXIT("failed to recognize function pointer Load Inst\n");
		}
		if(IS_VARIABLE_OR_ARRAY_TT(taintee)) {
			continue;
		}
		else if(taintee.ttype == TType::Struct) {
			PropagateValue(module, LI, getSOFunctions(taintee.structOffset.first, taintee.structOffset.second));
		} else {
			ERROR_EXIT("Unexpected Scenario : Unknown TType value\n");
		}
	}
}

void FineCFG::PropagateValue(Module * module, Value * val, FuncSet funcs) {
	vector<Value *> worklist;
	Value * worker, * user;
	set<Value *> added;
	debug() << *val << "\n";
	// printValAsOperand("PropagateValue ", val, "\n");
	QUEUE_ITERATE(worklist, val, worker) {
		StoreInst * SI = dyn_cast<StoreInst>(worker);
		if(SI) {
			HandleTaintedStore(SI, funcs);			
			continue;
		}
		int num_users = 0;
		FOR_USERS_OF_VALUE(worker, user) {
			if(CallInst * ci = dyn_cast<CallInst>(user)) {
				handleCallInst(ci, worker, funcs);
				continue;
			}
			if(GlobalVariable * gv = dyn_cast<GlobalVariable>(user)) {
				debug() << "GlobalsToAnalyze " << gv->getName() << "\n";
				GlobalsToAnalyze.insert(gv);
				continue;
			}
			if(StoreInst * SI = dyn_cast<StoreInst>(user)) {
				if(SI->getPointerOperand() == worker)
					continue;
			}
			num_users++;
			if(findInSet(added, user))
				continue;

			debug() << "inserting value " << *user << "\n";
			added.insert(user);
			worklist.push_back(user);
		}
	}
	printValAsOperand("Completed PropagateValue ", val, "\n");
} 

void FineCFG::PropagateTaintedVars(Module * module) {
	while(taintedVars.size()) {
		map<Value *, FuncSet> toIterate = taintedVars;
		taintedVars.clear();
		for(auto ent : toIterate) {
			debug() << "PropagateTaintedVars\n";
			PropagateValue(module, ent.first, ent.second);
		}
		PropagatetaintedCallArgs(module);
		TaintedCallArgs.clear();
		AnalyzeGlobals();
		GlobalsToAnalyze.clear();
		PropagateStructOffsets(module);
		structOffsetMapping.clear();
	}
}

bool FineCFG::HandleTaintedStore(StoreInst * SI, FuncSet funcs) {
	MemTaintee taintee;
	Value * ptr = SI->getPointerOperand();
	Instruction * tmp = NULL;	
	if(ConstantExpr * CE = dyn_cast<ConstantExpr>(ptr)) {
		ptr = tmp = CE->getAsInstruction();
	}
	taintedStores.insert(SI);
	bool res = CheckRelevantPtr(ptr, taintee);
	debug() << "store result of CheckRelevantPtr is " << res << "\n";
	if(tmp && !tmp->getParent()) tmp->dropAllReferences();
	if(!res) return false;
	if(IS_VARIABLE_OR_ARRAY_TT(taintee)) {
		addSetToSet(taintedVars[taintee.val], funcs);
	} else if(taintee.ttype == TType::Struct) {
		debug() << "adding to struct offset mapping " << *SI << "\n";
		addSetToSet(structOffsetMapping[taintee.structOffset], funcs);
	} else {
		ERROR_EXIT("Unexpected Scenario : Unknown TType value\n");
	}
	return true;
}

bool FineCFG::HandleTaintedLoad(LoadInst * LI, MemTaintee & taintee) {
	Value * ptr = LI->getPointerOperand();
	Instruction * tmp = NULL;
	if(ConstantExpr * CE = dyn_cast<ConstantExpr>(ptr)) {
		ptr = tmp = CE->getAsInstruction();
	}
	bool res = CheckRelevantPtr(ptr, taintee);
	if(tmp && !tmp->getParent()) tmp->dropAllReferences();
	return res;
}

bool FineCFG::CheckRelevantPtr(Value * ptr, MemTaintee & taintee) {
	/*if(CheckMemVar(ptr)) {
		taintee.ttype = TType::Variable;
		taintee.val = ptr;
		return true;
	}*/

	if(Value * castedOp = getCastedOperands(ptr))
		ptr = castedOp;
	GetElementPtrInst * GI = dyn_cast<GetElementPtrInst>(ptr);
	taintee.ttype = TType::None;
	if(!GI) return false;
	Value * gptr = GI->getPointerOperand();
	PointerType * pty = dyn_cast<PointerType>(gptr->getType());
	if(!pty) return false;
	StructType * st;
	int offset;
	debug() << "calling CheckGEPStruct\n";
	if(CheckGEPStruct(GI, st, offset, DL)) {
		taintee.ttype = TType::Struct;
		taintee.structOffset = make_pair(st, offset);
		return true;
	} /*else if(pty->getContainedType(0)->isArrayTy() && CheckMemVar(gptr)) {
		taintee.ttype = TType::Array;
		taintee.val = gptr;
		return true;	
	}*/
	return false;
}

void FineCFG::HandleBitCast(BitCastInst * BI) {
	Type * srcTy = BI->getSrcTy();
	Type * destTy = BI->getDestTy();
	while(srcTy->isPointerTy() && destTy->isPointerTy()) {
		srcTy = srcTy->getContainedType(0);
		destTy = destTy->getContainedType(0);
	}

	StructType * to = dyn_cast<StructType>(srcTy);
	StructType * from = dyn_cast<StructType>(destTy);
	if(!to || !from)
		return;
	CastingMap[to].insert(from);
	CastingMap[from].insert(to);
}

void FineCFG::handleCallInst(CallInst * CI, Value * tainted, FuncSet funcs) {
	if(CI->isInlineAsm())
		return;
	if(IS_INDIRECT_CALL_SITE(CI) && CI->getCalledValue() == tainted) {
		handledIndCallInsts.insert(CI);
		addSetToSet(CallsToTargets[CI], funcs);
	}
	Value * arg;
	FOR_ARGS_IN_CI(CI, arg) {
		if(arg == tainted)
			TaintedCallArgs[make_pair(CI, idx)] = funcs;
	}
}

void FineCFG::PropagateBitCasts(Module * module) {
	GlobalVariable * gv;
	FOR_GLOBALS_IN_MODULE(module, gv) {
		if(!gv->hasInitializer()) continue;
		Constant * CC = gv->getInitializer();
		vector<Value *> worklist;
		Value * worker;
		QUEUE_ITERATE(worklist, CC, worker) {
			if(isa<GlobalObject>(worker))
				continue;
			if(ConstantExpr * CE = dyn_cast<ConstantExpr>(worker)) {
				Instruction * tmp = CE->getAsInstruction();
				if(BitCastInst * BI = dyn_cast<BitCastInst>(tmp)) {
					HandleBitCast(BI);
				}
				if(!tmp->getParent()) tmp->dropAllReferences();
			}
			User * user = dyn_cast<User>(worker);
	    	if(!user) continue;
	    	Value * use;
	    	FOR_OPERANDS_OF_VALUE(user, use) worklist.push_back(use);
		}
	}
	Function * F;
	BasicBlock * BB;
	Instruction * I;
	FOR_INSTS_IN_MODULE(module, F, BB, I) {
		vector<Instruction *> worklist;
		Instruction * worker;
		QUEUE_ITERATE(worklist, I, worker) {
			if(BitCastInst * BI = dyn_cast<BitCastInst>(worker)) {
				HandleBitCast(BI);
			}
			Value * operand;
			FOR_OPERANDS_OF_VALUE(worker, operand) {
				if(ConstantExpr * CE = dyn_cast<ConstantExpr>(operand)) {
					Instruction * tmp = CE->getAsInstruction();
					worklist.push_back(tmp);	
				}
			}
			if(!worker->getParent()) worker->dropAllReferences();
		}
	}
}

bool isCalled(Function * F) {
	Value * user;
	FOR_USERS_OF_VALUE(F, user) {
		Instruction * I = dyn_cast<Instruction>(user);
		if(!I) continue;
		CallInst * ci = dyn_cast<CallInst>(I);
		if(!ci) continue;
		if(ci->getCalledValue()->stripPointerCasts() == F) return true;
	}
	return false;
}

void FineCFG::PrintFuncTargets(Module * module) {
	Function * F;
	BasicBlock * BB;
	Instruction * I;
	int total = 0, num_targets = 0;
	MemTaintee t;
	debugVar = true;
	int num_indirect_call_sites = 0;
	FOR_FUNCS_IN_MODULE(module, F) {
		debug() << F->getName() << "{\n";
		if(F->isDeclaration())
		    debug() << "__External_Function__\n";
		int call_index = 0;
		FOR_INSTS_IN_FUNC(F, BB, I) {
		    if(CallInst * ci = dyn_cast<CallInst>(I)) {
		        if(ci->isInlineAsm())
		            continue;
		        call_index++;
		        // debug() << *ci << " ";
		        Value * ptr = ci->getCalledValue()->stripPointerCasts();
		        if(Function * target = dyn_cast<Function>(ptr)) {
		            // debug() << 1 << " -1\n"; // -1 means not indirect
		            debug() << "callsite:-1 1\n"; // -1 means not indirect
		            debug() << target->getName() << "\n";
		        } else {
		        	if(!findInSet(handledIndCallInsts, ci)) {
		        		// debug() << -1 << " " << num_indirect_call_sites << "\n";
		        		debug() << "callsite:" << num_indirect_call_sites << " " << -1 << "\n";
		        		num_indirect_call_sites++;
		        		continue;
		        	}
		            FuncSet targets = CallsToTargets[ci];
		            if(targets.size()) {
		            	total++;
		            	num_targets += targets.size();
		            } /*else if(CheckRelevantPtr(ptr, t)) {
		            	errs() << F->getName() << " " << call_index << "\n";
		            }*/
		            // debug() << targets.size() << " " << num_indirect_call_sites << "\n";
	        		debug() << "callsite:" << num_indirect_call_sites << " " << targets.size() << "\n";
		            num_indirect_call_sites++;
		            for(auto func : targets)
						debug() << func->getName() << "\n";
		        }
		    }
		}
		debug() << "}\n";
	}
	errs() << num_targets << " " << total << "\n";
	errs() << handledIndCallInsts.size() << "\n";
	errs() << num_indirect_call_sites << "\n";
}