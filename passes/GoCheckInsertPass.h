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

    void ensureTypes(Module &M);
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

    Value* getTOP(Module &M, IRBuilder<> &Builder);
    void emitRemark(Instruction *I, StringRef Message);
};

} // namespace llvm

#endif
