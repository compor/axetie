
#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


using namespace llvm;

namespace {

struct Axetie : public FunctionPass {
    static char ID;

    Axetie() : FunctionPass(ID) {}

    bool runOnFunction(Function &f) override {
        errs() << "Axetie pass : ";
        errs() << " function name : ";
        errs().write_escaped(f.getName());
        auto ret_type = f.getReturnType();
        errs() << "\twith ret type : ";
        ret_type->print(errs());
        errs() << "\n";

        return false;
    }
};

} // namespace unnamed end


char Axetie::ID = 0;
static RegisterPass<Axetie> X("axetie", "Axetie Pass", false, false);


static void registerAxetiePass(const PassManagerBuilder &Builder,
                               legacy::PassManagerBase &PM) {
  PM.add(new Axetie());

  return;
}


static RegisterStandardPasses RegisterAxetiePass(
  PassManagerBuilder::EP_EarlyAsPossible, registerAxetiePass);

