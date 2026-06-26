#include "GoInitPass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

static cl::opt<int> GoTier("go-tier", cl::init(0),
  cl::desc("GOIR enforcement tier (0: diag, 1: hybrid, 2: cap)"));

cl::opt<int> GoInitPass::GoProvPolicy("go-prov-policy", cl::init(0),
  cl::desc("GOIR provenance policy (0: pnvi-plain, 1: pnvi-ae)"));

namespace llvm {

  void GoInitPass::ensureTypes(Module &M) {
    if (GradeTy) return;

    LLVMContext &Ctx = M.getContext();
    GradeTy = StructType::create(Ctx, {
        Type::getInt64Ty(Ctx), // base
        Type::getInt64Ty(Ctx), // end
        Type::getInt32Ty(Ctx), // alloc_id
        Type::getInt32Ty(Ctx), // epoch
        Type::getInt32Ty(Ctx), // perms
        Type::getInt32Ty(Ctx), // prov_tag
        Type::getInt64Ty(Ctx), // alias_tok
        Type::getInt32Ty(Ctx)  // flags
    }, "go.grade");

    Type *PtrTy = PointerType::getUnqual(Ctx);
    Type *SizeTy = Type::getInt64Ty(Ctx);

    GradeFromAllocaFn = M.getOrInsertFunction("llvm.go.grade_from_alloca", GradeTy, PtrTy, SizeTy);
    GradeFromMallocFn = M.getOrInsertFunction("llvm.go.grade_from_malloc", GradeTy, PtrTy, SizeTy);
  }

  void GoInitPass::emitRemark(Instruction *I, StringRef Message) {
    LLVMContext &Ctx = I->getContext();
    Ctx.diagnose(OptimizationRemark(
        "go-init", "Remark", I->getDebugLoc(), I->getParent()) << Message);
  }

  PreservedAnalyses GoInitPass::run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "GoInitPass running on module: " << M.getName() << " with tier " << GoTier << " and prov policy " << GoProvPolicy << "\n";

    M.addModuleFlag(Module::Error, "go-tier", (int)GoTier);
    M.addModuleFlag(Module::Error, "go-prov-policy", (int)GoProvPolicy);
    ensureTypes(M);

    for (Function &F : M) {
      for (BasicBlock &BB : F) {
        for (Instruction &I : llvm::make_early_inc_range(BB)) {
          IRBuilder<> Builder(&I);
          if (auto *AI = dyn_cast<AllocaInst>(&I)) {
            if (auto *Next = AI->getNextNode())
              Builder.SetInsertPoint(Next);
            else
              Builder.SetInsertPoint(&BB);

            Type *AllocTy = AI->getAllocatedType();
            const DataLayout &DL = M.getDataLayout();
            Value *Size = nullptr;

            if (AllocTy->isSized()) {
              uint64_t ElemSize = DL.getTypeAllocSize(AllocTy);
              Value *ArraySize = AI->getArraySize();
              if (auto *CI = dyn_cast<ConstantInt>(ArraySize)) {
                Size = ConstantInt::get(Type::getInt64Ty(M.getContext()), CI->getZExtValue() * ElemSize);
              } else {
                Value *CastArraySize = Builder.CreateZExtOrTrunc(ArraySize, Type::getInt64Ty(M.getContext()));
                Size = Builder.CreateMul(CastArraySize, ConstantInt::get(Type::getInt64Ty(M.getContext()), ElemSize));
                emitRemark(AI, "Dynamic alloca size; using dynamic grade init");
              }
            } else {
              Size = ConstantInt::get(Type::getInt64Ty(M.getContext()), -1ULL);
              emitRemark(AI, "Unknown alloca size; using TOP grade");
            }

            Builder.CreateCall(GradeFromAllocaFn, {AI, Size}, "g_p");
          } else if (auto *CI = dyn_cast<CallInst>(&I)) {
            Function *CalledFn = CI->getCalledFunction();
            if (!CalledFn) continue;

            if (CalledFn->getName() == "malloc") {
              if (auto *Next = CI->getNextNode())
                Builder.SetInsertPoint(Next);
              else
                Builder.SetInsertPoint(&BB);

              Value *Size = CI->getArgOperand(0);
              Builder.CreateCall(GradeFromMallocFn, {CI, Size}, "g_p");
            } else if (CalledFn->getName() == "calloc") {
              if (auto *Next = CI->getNextNode())
                Builder.SetInsertPoint(Next);
              else
                Builder.SetInsertPoint(&BB);

              Value *NMemb = CI->getArgOperand(0);
              Value *Size = CI->getArgOperand(1);
              Value *TotalSize = Builder.CreateMul(NMemb, Size);
              Builder.CreateCall(GradeFromMallocFn, {CI, TotalSize}, "g_p");
            } else if (CalledFn->getName() == "realloc") {
              if (auto *Next = CI->getNextNode())
                Builder.SetInsertPoint(Next);
              else
                Builder.SetInsertPoint(&BB);

              Value *Size = CI->getArgOperand(1);
              Builder.CreateCall(GradeFromMallocFn, {CI, Size}, "g_p");
            }
          }
        }
      }
    }

    return PreservedAnalyses::none();
  }
} // namespace llvm
