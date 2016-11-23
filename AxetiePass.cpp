
#include "llvm/Pass.h"
// using llvm::RegisterPass
// using llvm::ModulePass

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/LegacyPassManager.h"
// using llvm::legacy::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Support/raw_ostream.h"
// using llvm::errs



namespace {

struct Axetie : public llvm::ModulePass {
    static char ID;

      Axetie() : llvm::ModulePass(ID) {}

      bool runOnModule(llvm::Module &f) override {
        llvm::errs() << "Axetie pass : \n";

        return false;
    }
};

} // namespace unnamed end


char Axetie::ID = 0;
static llvm::RegisterPass<Axetie> X("axetie", "Axetie Pass", false, false);


static void registerAxetiePass(const llvm::PassManagerBuilder &Builder,
                               llvm::legacy::PassManagerBase &PM) {
  PM.add(new Axetie());

  return;
}


static llvm::RegisterStandardPasses RegisterAxetiePass(
  llvm::PassManagerBuilder::EP_EarlyAsPossible, registerAxetiePass);

