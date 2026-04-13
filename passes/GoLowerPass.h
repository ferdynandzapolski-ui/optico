#ifndef GO_LOWER_PASS_H
#define GO_LOWER_PASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace llvm {

struct GoLowerPass : public PassInfoMixin<GoLowerPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
