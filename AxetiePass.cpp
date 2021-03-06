
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

#ifndef NDEBUG
#include "llvm/IR/Verifier.h"
// using llvm::verifyModule
#endif // NDEBUG

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

#include "llvm/ADT/Twine.h"
// using llvm::Twine

#include "llvm/Support/CommandLine.h"
// using llvm::cl::list
// using llvm::cl::desc
// using llvm::cl::value_desc
// using llvm::cl::Required
// using llvm::cl::OneOrMore

#include "llvm/Support/raw_ostream.h"
// using llvm::outs
// using llvm::errs
// using llvm::dbgs

#include "llvm/Support/Debug.h"
// using DEBUG macros

#include <string>
// using std::string

#include <utility>
// using std::pair

#include <cassert>
// using assert

#define DEBUG_TYPE "axetie"

#ifndef NDEBUG
#define PLUGIN_OUT llvm::outs()
//#define PLUGIN_OUT llvm::nulls()

// convenience macro when building against a NDEBUG LLVM
#undef DEBUG
#define DEBUG(X)                                                               \
  do {                                                                         \
    X;                                                                         \
  } while (0);
#else // NDEBUG
#define PLUGIN_OUT llvm::dbgs()
#endif // NDEBUG

#define PLUGIN_ERR llvm::errs()

llvm::cl::list<std::string>
    atexit_handler_func_names("a",
                              llvm::cl::desc("Specify atexit handler name"),
                              llvm::cl::value_desc("function name"),
                              llvm::cl::Required, llvm::cl::OneOrMore);

namespace {

struct Axetie : public llvm::ModulePass {
  static char ID;
  llvm::LLVMContext *cur_context;

  const char *atexit_func_name = "atexit";
  const char *atexit_func_rc_suffix = "_rc";

  Axetie() : llvm::ModulePass(ID) { cur_context = nullptr; }

  decltype(auto) getEntryFunction(const llvm::Module &module) const {
    decltype(&*module.begin()) entry = nullptr;

    for (auto &CurFunc : module) {
      if (CurFunc.getName().compare("main") == 0) {
        entry = &CurFunc;
        break;
      }
    }

    return entry;
  }

  decltype(auto) createAtexitProto(llvm::Module &module) {
    assert(nullptr != cur_context);

    // TODO there should be a more elaborate type check here
    auto atexit_found = module.getFunction(atexit_func_name);

    if (!atexit_found) {
      auto arg_func_ret_ty = llvm::Type::getVoidTy(*cur_context);
      auto arg_func_ty =
          llvm::FunctionType::get(arg_func_ret_ty, false)->getPointerTo();

      auto ret_ty = llvm::Type::getInt32Ty(*cur_context);
      auto atexit_ty = llvm::FunctionType::get(ret_ty, arg_func_ty, false);

      atexit_found =
          llvm::Function::Create(atexit_ty, llvm::GlobalValue::ExternalLinkage,
                                 atexit_func_name, &module);
    }

    DEBUG(atexit_found->print(PLUGIN_OUT));

    return atexit_found;
  }

  decltype(auto) createExitHandlerProto(const llvm::StringRef &name,
                                        llvm::Module &module) {
    assert(nullptr != cur_context);

    auto ret_ty = llvm::Type::getVoidTy(*cur_context);
    auto atexit_handler_ty = llvm::FunctionType::get(ret_ty, false);

    auto atexit_handler = llvm::Function::Create(
        atexit_handler_ty, llvm::GlobalValue::ExternalLinkage, name, &module);

    DEBUG(atexit_handler->print(PLUGIN_OUT));

    return atexit_handler;
  }

  decltype(auto) createAtexitCall(const llvm::StringRef &name,
                                  llvm::Module &module) {
    assert(name.compare(atexit_func_name) != 0);

    auto handler = this->createExitHandlerProto(name, module);

    auto params = {static_cast<llvm::Value *>(handler)};
    auto atexit = this->createAtexitProto(module);

    auto call = llvm::CallInst::Create(
        atexit, params,
        llvm::Twine(llvm::StringRef(atexit_func_name), atexit_func_rc_suffix));

    DEBUG(call->print(PLUGIN_OUT));
    DEBUG(PLUGIN_OUT << "\n");

    return std::make_pair(call, handler);
  }

  bool addAtexitCall(const llvm::ArrayRef<const char *> &names,
                     llvm::Instruction &insert_pos) {
    bool is_modified = false;
    bool is_added = false;

    auto cur_module = insert_pos.getParent()->getParent()->getParent();
    assert(nullptr != cur_module);

    for (const auto &name : names) {
      auto r = createAtexitCall(name, *cur_module);
      auto call = r.first;
      auto handler = r.second;
      call->insertBefore(&insert_pos);

      cur_module->getOrInsertFunction(handler->getName(),
                                      handler->getFunctionType());
      is_modified = true;

      if (!is_added) {
        is_added = true;

        cur_module->getOrInsertFunction(
            call->getCalledFunction()->getName(),
            call->getCalledFunction()->getFunctionType());
      }
    }

    return is_modified;
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();

    return;
  }

  bool runOnModule(llvm::Module &cur_module) override {
    DEBUG(PLUGIN_OUT << "Axetie pass : \n");

    bool is_modified = false;

    cur_context = &cur_module.getContext();

    auto entry = getEntryFunction(cur_module);
    if (!entry)
      return false;

    DEBUG(PLUGIN_OUT << entry->getName() << "\n");

    auto insertion_pt = const_cast<llvm::Function *>(entry)
                            ->getEntryBlock()
                            .getFirstInsertionPt();

    for (const auto &ahandler_name : atexit_handler_func_names)
      is_modified = addAtexitCall({ahandler_name.c_str()}, *insertion_pt);

#ifndef NDEBUG
    llvm::verifyModule(cur_module, &(PLUGIN_ERR));
#endif // NDEBUG

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

static llvm::RegisterStandardPasses
    RegisterAxetiePass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                       registerAxetiePass);
