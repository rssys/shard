using namespace llvm;
using namespace std;

typedef vector<Type *> Signature;
#define IS_FUNC_PTR_TYPE(ty) (ty->isPointerTy() && ty->getContainedType(0)->isFunctionTy())

Signature getSignature(FunctionType * ty) {
	Signature sign;
	sign.push_back(ty->getReturnType());
	for(unsigned i = 0; i < ty->getNumParams(); i++) {
		sign.push_back(ty->getParamType(i));
	}
	return sign;
}

Signature getSignature(Type * ty) {
	if(!IS_FUNC_PTR_TYPE(ty)) {
		errs() << "getSignature called on type " << *ty << "\n";
		assert(0);
	}
	return getSignature(dyn_cast<FunctionType>(ty->getContainedType(0)));
}

Signature getSignature(Function * F) {
	return getSignature(F->getFunctionType());
}

string getSignStr(Signature sign) {
	string rso_str;
	raw_string_ostream rso(rso_str);

	string ret = "Return Type : ";
	sign[0]->print(rso);
	ret += rso.str() + " - Argument Types : ";
	rso_str = "";
	for(unsigned i = 1; i < sign.size(); i++) {
		sign[i]->print(rso);
		ret += rso.str();
		rso_str = "";
		if(i < sign.size() - 1)
			ret += ", ";
	}
	return ret;
}