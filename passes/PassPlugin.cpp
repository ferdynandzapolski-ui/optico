#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "GoInitPass.h"
#include "GoPropagatePass.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GOIRPasses", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "go-init") {
                    MPM.addPass(GoInitPass());
                    return true;
                  }
                  if (Name == "go-propagate") {
                    MPM.addPass(GoPropagatePass());
                    return true;
                  }
                  return false;
                });
          }};
}
