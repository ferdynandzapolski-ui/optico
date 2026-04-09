#ifndef GO_INIT_PASS_H
#define GO_INIT_PASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

struct GoInitPass : public PassInfoMixin<GoInitPass> {
  Type *GradeTy = nullptr;
  FunctionCallee GradeFromAllocaFn = nullptr;
  FunctionCallee GradeFromMallocFn = nullptr;

  void ensureTypes(Module &M);
  void emitRemark(Instruction *I, StringRef Message);
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif
