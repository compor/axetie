
#include "llvm/Pass.h"
// using llvm::RegisterPass
// using llvm::ModulePass

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

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
    llvm::LLVMContext *CurContext;

    Axetie() : llvm::ModulePass(ID) {
      CurContext = nullptr;
    }

    const llvm::Function *getEntryFunction(const llvm::Module &module) {
      for (auto &CurFunc : module) {
        if (CurFunc.getName().compare("main") == 0)
          return &CurFunc;
      }

      return nullptr;
    }

    bool runOnModule(llvm::Module &CurModule) override {
      llvm::errs() << "Axetie pass : \n";

      CurContext = CurModule.getContext();

      auto entry = getEntryFunction(CurModule);
      if (!entry) return false;

      llvm::errs() << entry->getName() << "\n";

      const auto &insertion_pt = entry->getEntryBlock().getFirstInsertionPt();

      return true;
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

