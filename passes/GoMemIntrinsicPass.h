#ifndef GO_MEM_INTRINSIC_PASS_H
#define GO_MEM_INTRINSIC_PASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace llvm {

struct GoMemIntrinsicPass : public PassInfoMixin<GoMemIntrinsicPass> {
    Type *GradeTy = nullptr;
    FunctionCallee MemcpyFn = nullptr;

    void ensureTypes(Module &M);
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
