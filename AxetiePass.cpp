
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

#include "llvm/IR/Instructions.h"
// using llvm::CallInst

#include "llvm/IR/LegacyPassManager.h"
// using llvm::legacy::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/GlobalValue.h"
// using llvm::GlobalValue

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include "llvm/ADT/Twine.h"
// using llvm::Twine

#include "llvm/Support/raw_ostream.h"
// using llvm::errs


#include <cassert>
// using assert


#define DEBUG_TYPE "axetie"


#ifndef NDEBUG
  #define PLUGIN_OUT llvm::outs()
  //#define PLUGIN_OUT llvm::nulls()
#else // NDEBUG
  #define PLUGIN_OUT llvm::dbgs()
#endif // NDEBUG

#define PLUGIN_ERR llvm::errs()


namespace {

  struct Axetie : public llvm::ModulePass {
    static char ID;
    llvm::LLVMContext *CurContext;

    const char *atexit_func_name = "atexit";
    const char *atexit_func_rc_suffix = "_rc";

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

    llvm::Function *createAtexitProto() {
      assert(nullptr != CurContext);

      auto arg_func_ret_ty = llvm::Type::getVoidTy(*CurContext);
      auto arg_func_ty = llvm::FunctionType::get(arg_func_ret_ty, false);

      auto ret_ty = llvm::Type::getInt32Ty(*CurContext);
      auto atexit_ty = llvm::FunctionType::get(ret_ty, false);

      auto atexit = llvm::Function::Create(atexit_ty,
                                           llvm::GlobalValue::ExternalLinkage,
                                           atexit_func_name);

      atexit->print(PLUGIN_OUT);

      return atexit;
    }

    llvm::Function *createExitHandlerProto(const llvm::StringRef &name) {
      assert(nullptr != CurContext);

      auto ret_ty = llvm::Type::getVoidTy(*CurContext);
      auto atexit_handler_ty = llvm::FunctionType::get(ret_ty, false);

      auto atexit_handler = llvm::Function::Create(atexit_handler_ty,
                                                   llvm::GlobalValue::ExternalLinkage,
                                                   name);

      atexit_handler->print(PLUGIN_OUT);

      return atexit_handler;
    }

    llvm::CallInst *createAtexitCall(const llvm::StringRef &name) {
      assert(name.compare(atexit_func_name) != 0);

      auto params = {
        static_cast<llvm::Value*>(this->createExitHandlerProto(name)) };
      auto atexit = this->createAtexitProto();

      auto call = llvm::CallInst::Create(atexit, params,
                                         llvm::Twine(llvm::StringRef(atexit_func_name),
                                                     atexit_func_rc_suffix));

      call->print(PLUGIN_OUT);

      return call;
    }

    bool addAtexitCall(const llvm::ArrayRef<const char *> names, llvm::Instruction *insert_pos) {
      bool modified = false;

      for (const auto &name : names) {
        auto call = createAtexitCall(name);
        insert_pos->insertBefore(call);
        modified = true;
      }

      return modified;
    }

    bool runOnModule(llvm::Module &CurModule) override {
      PLUGIN_OUT << "Axetie pass : \n";

      CurContext = &CurModule.getContext();

      auto entry = getEntryFunction(CurModule);
      if (!entry) return false;

      PLUGIN_OUT << entry->getName() << "\n";
      createAtexitCall("foo");

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

