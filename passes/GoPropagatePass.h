#ifndef GO_PROPAGATE_PASS_H
#define GO_PROPAGATE_PASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include <map>

namespace llvm {

struct GoPropagatePass : public PassInfoMixin<GoPropagatePass> {
    Type *GradeTy = nullptr;
    FunctionCallee GepGradeFn = nullptr;
    FunctionCallee JoinGradeFn = nullptr;

    void ensureTypes(Module &M);
    Value* getTOP(Module &M, IRBuilder<> &Builder);
    void emitRemark(Instruction *I, StringRef Message);
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
