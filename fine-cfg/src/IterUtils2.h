#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

#define INITIALIZE_FUNC_ITER(m_it, F) \
Module::iterator m_it = module->begin(); \
Function * F = &*m_it

Function * getNextFunc(Module * M, Module::iterator * m_it) {
	(*m_it)++;
	if(*m_it != M->end())
		return &**m_it;
	return NULL;
}

#define INITIALIZE_INST_ITER(m_it, f_it, b_it, inst) \
Module::iterator m_it = module->begin(); \
Function::iterator f_it = (&*m_it)->begin(); \
BasicBlock::iterator b_it = (&*f_it)->begin(); \
Instruction * inst = &*b_it

Instruction * getNextInst(Module * M, Module::iterator * m_it, Function::iterator * f_it, BasicBlock::iterator * b_it) {
	BasicBlock * BB = &**f_it;
	Function * F = &**m_it;
	(*b_it)++;
	if(*b_it != BB->end())
		return &**b_it;
	(*f_it)++;
	if(*f_it != F->end()) {
		(*b_it) = (&**f_it)->begin();
		return &**b_it;
	}
	
	while(true) {
		(*m_it)++;
		if((&**m_it)->isDeclaration())
			continue;
		break;
	}

	if(*m_it != M->end()) {
		*f_it = (&**m_it)->begin();
		(*b_it) = (&**f_it)->begin();
		return &**b_it;		
	}

	return NULL;
}

#define INITIALIZE_FUNC_INST_ITER(F, f_it, b_it, inst) \
Function::iterator f_it = F->begin(); \
BasicBlock::iterator b_it = (&*f_it)->begin(); \
Instruction * inst = &*b_it

Instruction * getNextInst(Function * F, Function::iterator * f_it, BasicBlock::iterator * b_it) {
	BasicBlock * BB = &**f_it;
	(*b_it)++;
	if(*b_it != BB->end())
		return &**b_it;
	(*f_it)++;
	if(*f_it != F->end()) {
		(*b_it) = (&**f_it)->begin();
		return &**b_it;
	}
	return NULL;
}

#define INITIALIZE_FUNC_ARG_ITER(F, a_it, arg) \
Function::arg_iterator a_it = F->arg_begin(); \
Value * arg = a_it != F->arg_end() ? &*a_it : NULL;

Value * getNextArg(Function * F, Function::arg_iterator * a_it) {
	(*a_it)++;
	if(*a_it != F->arg_end())
		return &**a_it; 
	return NULL;
}
