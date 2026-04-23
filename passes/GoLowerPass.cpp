#include "GoLowerPass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace llvm {

PreservedAnalyses GoLowerPass::run(Module &M, ModuleAnalysisManager &AM) {
    auto ReplaceCall = [&](StringRef IntrinsicName, StringRef RuntimeName) {
        Function *Intrinsic = M.getFunction(IntrinsicName);
        if (!Intrinsic) return;
        FunctionCallee RuntimeFn = M.getOrInsertFunction(RuntimeName, Intrinsic->getFunctionType());
        for (User *U : llvm::make_early_inc_range(Intrinsic->users())) {
            if (auto *CI = dyn_cast<CallInst>(U)) {
                CI->setCalledFunction(RuntimeFn);
            }
        }
        Intrinsic->eraseFromParent();
    };

    ReplaceCall("llvm.go.check_load", "__go_check_load");
    ReplaceCall("llvm.go.check_store", "__go_check_store");
    ReplaceCall("llvm.go.check_free", "__go_check_free");
    ReplaceCall("llvm.go.grade_from_alloca", "__go_grade_from_alloca");
    ReplaceCall("llvm.go.grade_from_malloc", "__go_grade_from_malloc");
    ReplaceCall("llvm.go.gep_grade", "__go_gep_grade");
    ReplaceCall("llvm.go.join_grade", "__go_join_grade");
    ReplaceCall("llvm.go.prov_expose", "__go_prov_expose");
    ReplaceCall("llvm.go.inttoptr_resolve", "__go_inttoptr_resolve");

    return PreservedAnalyses::none();
}

} // namespace llvm
