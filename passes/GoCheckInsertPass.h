#ifndef GO_CHECK_INSERT_PASS_H
#define GO_CHECK_INSERT_PASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/Statistic.h"

namespace llvm {

struct GoCheckInsertPass : public PassInfoMixin<GoCheckInsertPass> {
    Type *GradeTy = nullptr;
    FunctionCallee CheckLoadFn = nullptr;
    FunctionCallee CheckStoreFn = nullptr;
    FunctionCallee CheckFreeFn = nullptr;
    FunctionCallee MemcpyFn = nullptr;
    FunctionCallee MemmoveFn = nullptr;
    FunctionCallee MemsetFn = nullptr;

    void ensureTypes(Module &M);
    Value* getTOP(Module &M, IRBuilder<> &Builder);
    void emitRemark(Instruction *I, StringRef Message);
    Value* getGrade(Instruction *I, StringRef Kind = "go.grade");
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
