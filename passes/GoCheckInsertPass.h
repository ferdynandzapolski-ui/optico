#ifndef GO_CHECK_INSERT_PASS_H
#define GO_CHECK_INSERT_PASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace llvm {

struct GoCheckInsertPass : public PassInfoMixin<GoCheckInsertPass> {
    Type *GradeTy = nullptr;
    FunctionCallee CheckLoadFn = nullptr;
    FunctionCallee CheckStoreFn = nullptr;
    FunctionCallee CheckFreeFn = nullptr;
    FunctionCallee ShadowStoreFn = nullptr;
    FunctionCallee ShadowLoadFn = nullptr;
    FunctionCallee ProvExposeFn = nullptr;
    FunctionCallee IntToPtrResolveFn = nullptr;

    void ensureTypes(Module &M);
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
