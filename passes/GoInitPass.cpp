#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct GoInitPass : public PassInfoMixin<GoInitPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "GoInitPass running on module: " << M.getName() << "\n";
    return PreservedAnalyses::all();
  }
};
} // end anonymous namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GoInitPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "go-init") {
                    MPM.addPass(GoInitPass());
                    return true;
                  }
                  return false;
                });
          }};
}
