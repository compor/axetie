
#include "llvm/Pass.h"
// using llvm::RegisterPass
// using llvm::ModulePass
// using llvm::AnalysisUsage

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

#include "llvm/IR/Value.h"
// using llvm::Value

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
// using llvm::outs
// using llvm::errs
// using llvm::dbgs


#include <utility>
// using std::pair

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
    llvm::LLVMContext *cur_context;

    const char *atexit_func_name = "atexit";
    const char *atexit_func_rc_suffix = "_rc";

    Axetie() : llvm::ModulePass(ID) {
      cur_context = nullptr;
    }

    const llvm::Function *getEntryFunction(const llvm::Module &module) {
      for (auto &CurFunc : module) {
        if (CurFunc.getName().compare("main") == 0)
          return &CurFunc;
      }

      return nullptr;
    }

    llvm::Function *createAtexitProto() {
      assert(nullptr != cur_context);

      auto arg_func_ret_ty = llvm::Type::getVoidTy(*cur_context);
      auto arg_func_ty = llvm::FunctionType::get(arg_func_ret_ty, false)->getPointerTo();

      auto ret_ty = llvm::Type::getInt32Ty(*cur_context);
      auto atexit_ty = llvm::FunctionType::get(ret_ty, arg_func_ty, false);

      auto atexit = llvm::Function::Create(atexit_ty,
                                           llvm::GlobalValue::ExternalLinkage,
                                           atexit_func_name);

      atexit->print(PLUGIN_OUT);

      return atexit;
    }

    llvm::Function *createExitHandlerProto(const llvm::StringRef &name) {
      assert(nullptr != cur_context);

      auto ret_ty = llvm::Type::getVoidTy(*cur_context);
      auto atexit_handler_ty = llvm::FunctionType::get(ret_ty, false);

      auto atexit_handler = llvm::Function::Create(atexit_handler_ty,
                                                   llvm::GlobalValue::ExternalLinkage,
                                                   name);

      atexit_handler->print(PLUGIN_OUT);

      return atexit_handler;
    }

    std::pair<llvm::CallInst *, llvm::Function *>
    createAtexitCall(const llvm::StringRef &name) {
      assert(name.compare(atexit_func_name) != 0);

      auto handler = this->createExitHandlerProto(name);

      auto params = {
        static_cast<llvm::Value*>(handler) };
      auto atexit = this->createAtexitProto();

      auto call = llvm::CallInst::Create(atexit, params,
                                         llvm::Twine(llvm::StringRef(atexit_func_name),
                                                     atexit_func_rc_suffix));

      call->print(PLUGIN_OUT);
      PLUGIN_OUT << "\n";

      return std::make_pair(call, handler);
    }

    bool addAtexitCall(const llvm::ArrayRef<const char *> names,
                       llvm::Instruction *insert_pos) {
      bool is_modified = false;
      bool is_added = false;

      for (const auto &name : names) {
        auto r = createAtexitCall(name);
        auto call = r.first;
        auto handler = r.second;
        call->insertBefore(insert_pos);

        auto cur_module = call->getParent()->getParent()->getParent();

        cur_module->getOrInsertFunction(handler->getName(),
                                        handler->getFunctionType());
        is_modified = true;

        if (!is_added) {
          is_added = true;

          cur_module->getOrInsertFunction(
            call->getCalledFunction()->getName(), call->getFunctionType());
        }
      }

      return is_modified;
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.setPreservesCFG();

      return;
    }

    bool runOnModule(llvm::Module &cur_module) override {
      PLUGIN_OUT << "Axetie pass : \n";

      bool is_modified = false;

      cur_context = &cur_module.getContext();

      auto entry = getEntryFunction(cur_module);
      if (!entry) return false;

      PLUGIN_OUT << entry->getName() << "\n";

      auto insertion_pt = const_cast<llvm::Function*>(entry)->getEntryBlock().getFirstInsertionPt();

      is_modified = addAtexitCall({ "qux", "baz" }, &*insertion_pt);

      //cur_module.dump();

      return is_modified;
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

