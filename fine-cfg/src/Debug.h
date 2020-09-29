using namespace llvm;
using namespace std;

bool debugVar = false; 
class debug {
    public:
    debug() {
        if(debugVar)
            ignore = false;
        else
            ignore = true;
    }
    template<class T>
    debug & operator << (const T &x) {
        if(!ignore)
            errs() << x;
        return *this;
    }

    private:
        bool ignore;
};

void initDebug() {
  char * value = getenv("DEBUG_VAR");
  if(!value)
      return;
  debugVar = true;
}

void printValAsOperand(string before, Value * val, string after) {
    debug() << before;
    if(debugVar)
        val->printAsOperand(errs(), false); 
    debug() << after;
}

void printInst(Instruction * I) {
  debug() << *I << " is the inst "; 
  if(I->getParent()) {
    printValAsOperand(" in BB ", I->getParent(), ""); 
    if(I->getFunction())
      debug() << " " << I->getFunction()->getName() << " ";
  }
  debug() << "\n";
}
