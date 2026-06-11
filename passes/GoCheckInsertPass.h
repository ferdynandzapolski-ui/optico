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
    FunctionCallee ProvExposeFn = nullptr;
    FunctionCallee IntToPtrResolveFn = nullptr;
    FunctionCallee ShadowStoreFn = nullptr;
    FunctionCallee ShadowLoadFn = nullptr;

    void ensureTypes(Module &M);
    Value* getTOP(Module &M, IRBuilder<> &Builder);
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
