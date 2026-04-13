#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Module.h"
#include <cstdlib>

using namespace llvm;

namespace {
  struct GoPropagatePass : public FunctionPass {
    static char ID;
    GoPropagatePass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      double cheri_confidence = 0.0;

      // Attempt to read confidence from module flags or environment
      if (Metadata *MD = F.getParent()->getModuleFlag("goir.inv.cheri_elision_safe")) {
        if (auto *CI = mdconst::dyn_extract<ConstantFP>(MD)) {
          cheri_confidence = CI->getValueAPF().convertToDouble();
        }
      } else if (const char* env_conf = std::getenv("GOIR_CHERI_CONFIDENCE")) {
        cheri_confidence = std::atof(env_conf);
      }

      bool aggressive = cheri_confidence >= 0.995;

      if (aggressive) {
        errs() << "GoPropagate: Aggressive propagation enabled (confidence: " << cheri_confidence << ")\n";
      }

      // Implementation of propagation logic gated by 'aggressive'
      // ... (existing propagation code) ...

      return false;
    }
  };
}

char GoPropagatePass::ID = 0;
static RegisterPass<GoPropagatePass> X("go-propagate", "GoPropagate Pass", false, false);
