#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

static cl::opt<int> GoTier("go-tier", cl::init(0),
  cl::desc("GOIR enforcement tier (0: diag, 1: hybrid, 2: cap)"));

namespace {
struct GoInitPass : public PassInfoMixin<GoInitPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "GoInitPass running on module: " << M.getName() << " with tier " << GoTier << "\n";

    // Add module-level metadata to persist the chosen tier
    M.addModuleFlag(Module::Error, "go-tier", GoTier);

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
