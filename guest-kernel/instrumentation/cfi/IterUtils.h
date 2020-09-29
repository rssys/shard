#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

#define FOR_GLOBALS_IN_MODULE(M, gv) \
	Module::global_iterator g_it; \
	for(g_it = M->global_begin(), gv = &*g_it; g_it != M->global_end(); g_it++, gv = &*g_it)

#define FOR_FUNCS_IN_MODULE(M, F) \
	Module::iterator m_it; \
	for(m_it = M->begin(), F = &*m_it; m_it != M->end(); m_it++, F = &*m_it)


#define FOR_INSTS_IN_FUNC(F, BB, I) \
	Function::iterator f_it; \
	BasicBlock::iterator b_it; \
	for(f_it = F->begin(), BB = &*f_it; f_it != F->end(); f_it++, BB = &*f_it) \
		for(b_it = BB->begin(), I = &*b_it; b_it != BB->end(); b_it++, I = &*b_it)

#define FOR_INSTS_IN_MODULE(M, F, BB, I) \
	Module::iterator m_it; \
	Function::iterator f_it; \
	BasicBlock::iterator b_it; \
	for(m_it = M->begin(), F = &*m_it; m_it != M->end(); m_it++, F = &*m_it) \
		for(f_it = F->begin(), BB = &*f_it; f_it != F->end(); f_it++, BB = &*f_it) \
			for(b_it = BB->begin(), I = &*b_it; b_it != BB->end(); b_it++, I = &*b_it)

#define FOR_ARGS_IN_FUNC(F, arg) \
	for(Function::arg_iterator a_it = F->arg_begin(); a_it != F->arg_end(); a_it++, arg = &*a_it)

#define FOR_OPERANDS_OF_VALUE(user, use)\
	for(unsigned idx = 0; idx < user->getNumOperands() && (use = user->getOperand(idx)); idx++)

#define FOR_USERS_OF_VALUE(val, user)\
	Value::user_iterator ui;\
	for(ui = val->user_begin(); ui != val->user_end() && (user = *ui); ui++)

#define FOR_ARGS_IN_CI(CI, arg) \
	for(unsigned idx = 0; idx < CI->getNumArgOperands() && (arg = CI->getArgOperand(idx)); idx++)