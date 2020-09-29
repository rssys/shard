#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <string>

using namespace std;
using namespace llvm;

#define IS_FUNC_PTR_TYPE(ty) (ty->isPointerTy() && ty->getContainedType(0)->isFunctionTy())
#define IS_VARIABLE_OR_ARRAY_TT(taintee) (taintee.ttype == TType::Variable || taintee.ttype == TType::Array)
#define IS_INDIRECT_CALL_SITE(ci) (!isa<Function>(ci->getCalledValue()->stripPointerCasts()))


#define ERROR_EXIT(str)\
  errs() << str;\
  assert(0)

extern bool print_it;

string GetSrcInfo(Instruction * I) {
    const llvm::DebugLoc &debugInfo = I->getDebugLoc();
    if(!debugInfo) return "";
    string directory = debugInfo->getDirectory();
    string filePath = debugInfo->getFilename();
    int line = debugInfo->getLine();
    return directory + "/" + filePath + " lineno : " + to_string(line);
}

void printVal(string before, Value * val, string after) {
    errs() << before;
   	val->printAsOperand(errs(), false);        
    errs() << after;
}

bool CheckMemVar(Value * ptr) {
  if(isa<GlobalVariable>(ptr) || isa<AllocaInst>(ptr))
    return true;
  return false;
}

bool getGEPOffset(GetElementPtrInst * GI, int & offset, DataLayout * DL) { 
  unsigned OffsetBits = DL->getPointerTypeSizeInBits(GI->getType());
  APInt aio(OffsetBits, 0); 
  bool isConst = GI->accumulateConstantOffset(*DL, aio);
  offset = aio.getZExtValue();
  return isConst;
}

int getStructFieldOffset(StructType * ty, int field, DataLayout * DL) {
  const StructLayout * SL = DL->getStructLayout(ty);
  return SL->getElementOffset(field);
}

bool hasFuncPointerAtOffset(StructType * ty, int offset, DataLayout * DL) {
  for(unsigned i = 0; i < ty->getNumContainedTypes(); i++) {
    int curr = getStructFieldOffset(ty, i, DL);
    if(curr > offset) return false;
    if(curr < offset) continue;
    return IS_FUNC_PTR_TYPE(ty->getContainedType(i));
  }
  return false;
}

bool UpdateType(Type * prevTy, Type *& nextTy, int & field, Value * idxVal) {
  if(prevTy->isPointerTy() || prevTy->isArrayTy()) {
    nextTy = prevTy->getContainedType(0);
    return true;
  }
  if(!isa<StructType>(prevTy)) {
    errs() << "gep not into a struct type " << *prevTy << "\n";
    return false;
  }
  ConstantInt * idx = dyn_cast<ConstantInt>(idxVal);
  if(!idx) {
    ERROR_EXIT("Unexpected Scenario : Struct GEP has variable offset\n");
  }
  field = idx->getZExtValue();
  nextTy = prevTy->getContainedType(field);
  return true;
}

bool CheckGEPStruct(GetElementPtrInst * GI, StructType *& st, int & offset, DataLayout * DL) {
  Type * prevTy = GI->getPointerOperand()->getType();
  Type * nextTy;
  int field;
  for(auto index = GI->idx_begin(), end = GI->idx_end(); index != end; index++) {
    if(!UpdateType(prevTy, nextTy, field, *index)) {
      return false;
    }
    if(index + 1 == end) {//IS_FUNC_PTR_TYPE(nextTy)) {
      st = dyn_cast<StructType>(prevTy);
      if(!st) return false;   
      offset = getStructFieldOffset(st, field, DL);
      return true;
    }
    prevTy = nextTy;
  }
  return false;
}

Value * getCastedOperands(Value * val) {
  if(BitCastInst * BI = dyn_cast<BitCastInst>(val))
    return BI->getOperand(0);
  if(ConstantExpr * CE = dyn_cast<ConstantExpr>(val)) {
    Instruction * tmp = CE->getAsInstruction();
    if(BitCastInst * BI = dyn_cast<BitCastInst>(tmp))
      return BI->getOperand(0);
  }
  return NULL;
}

bool CheckStructLoad(CallInst * ci, DataLayout * DL) {
  Value * ptr = ci->getCalledValue();
  LoadInst * LI = dyn_cast<LoadInst>(ptr);
  if(!LI) return false;
  Value * lptr = LI->getPointerOperand();
  Instruction * tmp = NULL;
  if(ConstantExpr * CE = dyn_cast<ConstantExpr>(lptr))
    lptr = tmp = CE->getAsInstruction();
  if(Value * castedOp = getCastedOperands(ptr))
    lptr = castedOp;  
  GetElementPtrInst * GI = dyn_cast<GetElementPtrInst>(lptr);
  if(!GI) {
    if(tmp && !tmp->getParent()) tmp->dropAllReferences();
    return false;
  }
  PointerType * pty = dyn_cast<PointerType>(GI->getPointerOperand()->getType());
  if(!pty) {
    if(tmp && !tmp->getParent()) tmp->dropAllReferences();
    return false;
  }
  StructType * st;
  int offset;
  if(CheckGEPStruct(GI, st, offset, DL)) {
    if(tmp && !tmp->getParent()) tmp->dropAllReferences();
    return true;
  }
  if(tmp && !tmp->getParent()) tmp->dropAllReferences();
  return false;
}

Value * getArg(Function * func, int index) {
  int i = 0;
  for(auto arg = func->arg_begin(), argEnd = func->arg_end(); arg != argEnd; arg++){
    if(i == index) 
      return &*arg;
    i++;
  }
  return NULL;  
}
