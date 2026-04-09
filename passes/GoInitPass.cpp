#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>

using namespace llvm;

static cl::opt<int> GoTierOpt("go-tier", cl::desc("GOIR enforcement tier"),
                              cl::Hidden, cl::init(0));

namespace {
struct GoInitPass : public PassInfoMixin<GoInitPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    int EffectiveTier = GoTierOpt;

    // Check environment variable (useful for Clang integration)
    if (const char *EnvTier = std::getenv("GOIR_TIER")) {
      EffectiveTier = std::atoi(EnvTier);
    }

    // Check module flag (in case it was already set)
    if (auto *Flag = mdconst::extract_or_null<ConstantInt>(
            M.getModuleFlag("go-tier"))) {
      EffectiveTier = Flag->getZExtValue();
    }

    errs() << "GoInitPass running on module: " << M.getName()
           << " (Tier: " << EffectiveTier << ")\n";

    // Emit the tier as a module flag if not already present.
    if (!M.getModuleFlag("go-tier")) {
      M.addModuleFlag(Module::Error, "go-tier", EffectiveTier);
    }

    return PreservedAnalyses::all();
  }
};
} // end anonymous namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GoInitPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // Register for use with opt -passes=go-init
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "go-init") {
                    MPM.addPass(GoInitPass());
                    return true;
                  }
                  return false;
                });
            // Register for automatic execution in clang
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(GoInitPass());
                });
          }};
}
