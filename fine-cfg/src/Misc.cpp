#include "Misc.h"
#include "IterUtils.h"
#include "StdUtils.h"
#include "Utils.h"
#include "llvm/IR/GlobalVariable.h"


char Misc::ID = 0;
static RegisterPass<Misc> X("Misc", "Misc Pass", false, false);


typedef set<StructType *> StructSet;
typedef string Location;

#define LOCATION(name, line, fname, dirname) dirname + "/" + fname + "_" + to_string(line) + "_" + dirname

map<StructType *, Location> StructToLocation;
map<Location, StructSet> LocationToStructSet;

void printInfo(DIType * dt) {
	errs() << dt->getName() << " "
	 	   << dt->getLine() << " " 
	 	   << dt->getFilename() << " " 
	 	   << dt->getDirectory() << "\n";
}

void AddLocation(StructType * ST, DIType * DT) {
	Location location = LOCATION(DT->getName().str(), DT->getLine(), DT->getFilename().str(), DT->getDirectory().str());
	StructToLocation[ST] = location;
	LocationToStructSet[location].insert(ST);
}

StructType * getStructType(Type * ty) {
	if(StructType * st = dyn_cast<StructType>(ty))
		return st;
	else if(ty->isPointerTy() || ty->isArrayTy())
		return getStructType(ty->getContainedType(0)); 
	return NULL;
}

bool ignoreDT(DIType * dt) {
	return dt->getTag() == dwarf::DW_TAG_base_type 
		|| dt->getTag() == dwarf::DW_TAG_enumeration_type
		|| dt->getTag() == dwarf::DW_TAG_union_type
		|| dt->getTag() == dwarf::DW_TAG_volatile_type
		|| isa<DISubroutineType>(dt);
}

DIType * getBaseType(DIType * DT) {
	if(DIDerivedType * DDT = dyn_cast<DIDerivedType>(DT)) {
		if(DDT->getBaseType())
			return DDT->getBaseType().resolve();
		else return NULL;
	} else if(DICompositeType * DCT = dyn_cast<DICompositeType>(DT)) {
		if(DCT->getBaseType())
			return DCT->getBaseType().resolve();
		else return NULL;
	}
	return NULL;
}

vector<DIType *> getDTElems(DICompositeType * CT) {
	vector<DIType *> elems;
	DINodeArray DIA = CT->getElements();
	for(DINode * DN : DIA) {
		DIType * EDT = getBaseType(dyn_cast<DIDerivedType>(DN));
		elems.push_back(EDT);
	}
	return elems;
}

Value * curr;
#define ASSERT_AND_PRINT(cond, ERROR) \
if(!(cond)) { \
	errs() << *curr << "\n";\
	errs() << *ty << "\n";\
	errs() << *dt << "\n";\
	ERROR_EXIT(ERROR);\
}

#define RETURN_FALSE(ERROR) \
// errs() << *curr << "\n";\
// errs() << *ty << "\n";\
// errs() << *dt << "\n";\
// errs() << ERROR;\
return false

#define IS_PTR_TY(ty) (ty->isPointerTy() || ty->isArrayTy())
#define CHECK_INT(ty) (ty->isIntegerTy())
#define CHECK_INT_PTR(ty) (IS_PTR_TY(ty) && CHECK_INT(ty->getContainedType(0)))
#define CHECK_INT_OR_INT_PTR(ty) (CHECK_INT(ty) || CHECK_INT_PTR(ty))

// return true if DT is a basic type, typedef to a basic type or a pointer to a basic type
bool CheckBasicType(DIType * DT) {
	if(!DT) return true;
	if(DT->getTag() == dwarf::DW_TAG_pointer_type)
		return CheckBasicType(getBaseType(DT));
	else if(DT->getTag() == dwarf::DW_TAG_typedef || DT->getTag() == dwarf::DW_TAG_const_type)
		return CheckBasicType(getBaseType(DT));
	return DT->getTag() == dwarf::DW_TAG_base_type;
}

bool CheckStructOfInts(DICompositeType * CT) {
	vector<DIType *> elems = getDTElems(CT);
	for(unsigned i = 0; i < elems.size(); i++) {
		if(!CheckBasicType(elems[i])) return false;
	}
	return true;
}

bool getNextSCTys(Type * ty, DIType * dt, StructType *& st, DICompositeType *& ct) {
	DIType * underlying;
	if(ignoreDT(dt)) {
		return false;
		// RETURN_FALSE("IGNORED\n**************************\n");
	}
	else if(dt->getTag() == dwarf::DW_TAG_typedef || dt->getTag() == dwarf::DW_TAG_const_type) {
		if(!(underlying = getBaseType(dt))) {
			if(CHECK_INT_OR_INT_PTR(ty)) return false;
			RETURN_FALSE("NOT BASE TYPE\n**************************\n");
		}
		return getNextSCTys(ty, underlying, st, ct);
	} else if(dt->getTag() == dwarf::DW_TAG_pointer_type) {
		// ASSERT_AND_PRINT(isa<PointerType>(ty) || isa<ArrayType>(ty), "dt pointer, ty not pointer\n")
		if(!isa<PointerType>(ty) && !isa<ArrayType>(ty)) return false;
		if(!(underlying = getBaseType(dt))) {
			if(CHECK_INT_OR_INT_PTR(ty)) return false;
			RETURN_FALSE("NOT BASE TYPE\n**************************\n");
		}
		return getNextSCTys(ty->getContainedType(0), underlying, st, ct);
	} else if(dt->getTag() == dwarf::DW_TAG_array_type) {
		// ASSERT_AND_PRINT(isa<CompositeType>(ty), "dt composite, ty not composite\n")
		if(!(underlying = getBaseType(dt))) {
			RETURN_FALSE("NOT BASE TYPE\n**************************\n");
		}
		if(!isa<SequentialType>(ty) && !isa<PointerType>(ty)) {
			return false;
			// if(CheckBasicType(underlying)) return false;
			// if(underlying->getName() == "event_constraint") return false;
			// RETURN_FALSE("dt array, ty not sequential / pointer\n**************************\n");
		}
		return getNextSCTys(ty->getContainedType(0), underlying, st, ct);		
	} else if(dt->getTag() == dwarf::DW_TAG_structure_type) {
		if(!isa<StructType>(ty)) {
			return false;
			// RETURN_FALSE("NOT STRUCT TYPE\n**************************\n");
		}
		st = dyn_cast<StructType>(ty);
		ct = dyn_cast<DICompositeType>(dt);
		return true;
	}
	ASSERT_AND_PRINT(0, "Unhandled Case\n")
	return false;
}

#define ASSERT_AND_PRINT2(cond, ERROR)\
if(!(cond)) {\
	errs() << *DIA << "\n";\
	errs() << *ST << "\n";\
	errs() << ERROR;\
	assert(0);\
}

vector<pair<Type *, DIType *> > getStructDTElems(StructType * ST, DICompositeType * CT) {
	vector<pair<Type *, DIType *> > elems;
	DINodeArray DIA = CT->getElements();
	int idx = 0;
	for(DINode * DN : DIA) {
		Type * ET = ST->getContainedType(idx);
		ASSERT_AND_PRINT2((DN->getTag() == dwarf::DW_TAG_member), "")
		DIType * EDT = getBaseType(dyn_cast<DIDerivedType>(DN));
		elems.push_back(make_pair(ET, EDT));
		errs() << *ET << "\n";		
		errs() << *EDT << "\n";
		idx += 1;
	}
	errs() << DIA.size() << " " << ST->getNumContainedTypes() << "\n";
	ASSERT_AND_PRINT2(DIA.size() == ST->getNumContainedTypes(), "")
	return elems;
}

set<StructType *> seen;
void HandleStructDT(Type * ty, DIType * dt) {
	vector<pair<Type *, DIType *> > worklist;
	pair<Type *, DIType *> worker;
	QUEUE_ITERATE(worklist, make_pair(ty, dt), worker) {
		Type * currTy = worker.first;
		DIType * currDTy = worker.second;
		StructType * nextSTy;
		DICompositeType * nextCTy;
		if(!getNextSCTys(currTy, currDTy, nextSTy, nextCTy))
			continue;
		if(findInSet(seen, nextSTy)) continue;
		seen.insert(nextSTy);		
		AddLocation(nextSTy, nextCTy);
		// addVectToVect(worklist, getStructDTElems(nextSTy, nextCTy));
	}
}

void HandleGlobals(Module * module) {
	GlobalVariable * gv;
	FOR_GLOBALS_IN_MODULE(module, gv) {
		curr = gv;
		Type * ty = gv->getType();
		SmallVector<DIGlobalVariableExpression *, 1> dbgs;
		gv->getDebugInfo(dbgs);
		for(auto dbg : dbgs) {
			DIType * dt = dbg->getVariable()->getType().resolve();
			HandleStructDT(ty->getContainedType(0), dt);
		}
	}
}

#define GETDECVAL(decVal, MDVal)\
Metadata * tmp = dyn_cast<MetadataAsValue>(MDVal)->getMetadata();\
decVal = dyn_cast<ValueAsMetadata>(tmp)->getValue()

#define GETDILOCAL(DLV, MDVal)\
Metadata * MD = dyn_cast<MetadataAsValue>(ci->getOperand(1))->getMetadata();\
assert(isa<DILocalVariable>(MD));\
DLV = dyn_cast<DILocalVariable>(MD)

void HandleInsts(Module * module) {
	Function * F;
	BasicBlock * BB;
	Instruction * I;
	Value * decVal;
	DILocalVariable * DLV;
	FOR_INSTS_IN_MODULE(module, F, BB, I) {
		if(CallInst * ci = dyn_cast<CallInst>(I)) {
			curr = I;
			if(!ci->getCalledFunction()) continue;
			if(!(ci->getCalledFunction()->getName() == "llvm.dbg.declare")) continue;
			assert(isa<MetadataAsValue>(ci->getOperand(0)));
			assert(isa<MetadataAsValue>(ci->getOperand(1)));			
			GETDECVAL(decVal, ci->getOperand(0));
			GETDILOCAL(DLV, ci->getOperand(1));
			Type * ty = decVal->getType();
			DIType * dt = DLV->getType().resolve();
			HandleStructDT(ty->getContainedType(0), dt);
		}
	}
}


bool StructAtDifferentLevel(Type * ty1, Type * ty2) {
	while(IS_PTR_TY(ty1) && IS_PTR_TY(ty2)) {
		ty1 = ty1->getContainedType(0);
		ty2 = ty2->getContainedType(0);
	}
	if(isa<StructType>(ty1) && !isa<StructType>(ty2)) return true;
	if(isa<StructType>(ty2) && !isa<StructType>(ty1)) return true;
	return true;
}

void PropagateToContained() {
	for(auto entry : LocationToStructSet) {
		StructType * first = NULL;
		bool clear = true;
		for(auto ST : entry.second) {
			if(!ST->getNumContainedTypes()) {
				clear = false;
				break;
			}
			if(!first) {
				first = ST;
				continue;
			}
			if(ST->getNumContainedTypes() != first->getNumContainedTypes()) {
				clear = false;
				break;
			}
			for(unsigned i = 0; i < ST->getNumContainedTypes(); i++) {
				if(StructAtDifferentLevel(first->getContainedType(i), ST->getContainedType(i))) {
					clear = false;
				}
			}
		}
	}
}

bool Misc::runOnModule(llvm::Module& module) {
	DL = new DataLayout(&module);
	HandleGlobals(&module);
 	HandleInsts(&module);
	return false;
}