#include "GoCheckInsertPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static cl::opt<int> CheckTier("go-check-tier", cl::init(0),
  cl::desc("GOIR check tier (0: diag, 1: hybrid, 2: cap)"), cl::Hidden);

static cl::opt<int> ProvPolicy("go-prov-policy", cl::init(0),
  cl::desc("GOIR provenance policy (0: PNVI-plain, 1: PNVI-ae)"), cl::Hidden);

namespace llvm {

void GoCheckInsertPass::ensureTypes(Module &M) {
    if (CheckLoadFn) return;
    LLVMContext &Ctx = M.getContext();
    GradeTy = M.getTypeByName("go.grade");
    if (!GradeTy) {
        GradeTy = StructType::create(Ctx, {
            Type::getInt64Ty(Ctx), Type::getInt64Ty(Ctx),
            Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx),
            Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx),
            Type::getInt64Ty(Ctx), Type::getInt32Ty(Ctx)
        }, "go.grade");
    }

    Type *PtrTy = PointerType::getUnqual(Ctx);
    Type *SizeTy = Type::getInt64Ty(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);

    CheckLoadFn = M.getOrInsertFunction("llvm.go.check_load", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckStoreFn = M.getOrInsertFunction("llvm.go.check_store", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckFreeFn = M.getOrInsertFunction("llvm.go.check_free", Type::getVoidTy(Ctx), PtrTy, GradeTy);

    ShadowStoreFn = M.getOrInsertFunction("llvm.go.shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowLoadFn = M.getOrInsertFunction("llvm.go.shadow_load", GradeTy, PtrTy);
    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy, I32Ty);

    StructType *PtrGradeResTy = StructType::get(Ctx, {PtrTy, GradeTy});
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve", PtrGradeResTy, Type::getInt64Ty(Ctx), I32Ty, I32Ty);
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

    int Tier = CheckTier;
    if (auto *F = M.getModuleFlag("go-tier")) {
        if (auto *CI = mdconst::dyn_extract<ConstantInt>(F))
            Tier = CI->getZExtValue();
    }
    int Policy = ProvPolicy;
    if (auto *F = M.getModuleFlag("go-prov-policy")) {
        if (auto *CI = mdconst::dyn_extract<ConstantInt>(F))
            Policy = CI->getZExtValue();
    }

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : llvm::make_early_inc_range(BB)) {
                IRBuilder<> Builder(&I);

                auto getGrade = [&](Value *Ptr) -> Value* {
                    if (auto *Inst = dyn_cast<Instruction>(Ptr)) {
                        if (auto *MD = Inst->getMetadata("go.grade")) {
                            return cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();
                        }
                    }
                    return nullptr;
                };

                auto getTOP = [&]() {
                     LLVMContext &Ctx = M.getContext();
                     return ConstantStruct::get(cast<StructType>(GradeTy), {
                         ConstantInt::get(Type::getInt64Ty(Ctx), 0),
                         ConstantInt::get(Type::getInt64Ty(Ctx), -1ULL),
                         ConstantInt::get(Type::getInt32Ty(Ctx), 0),
                         ConstantInt::get(Type::getInt32Ty(Ctx), 0),
                         ConstantInt::get(Type::getInt32Ty(Ctx), 0xF),
                         ConstantInt::get(Type::getInt32Ty(Ctx), 0),
                         ConstantInt::get(Type::getInt64Ty(Ctx), 0),
                         ConstantInt::get(Type::getInt32Ty(Ctx), 0)
                     });
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    if (!G) G = getTOP();

                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});

                    if (Tier >= 1 && LI->getType()->isPointerTy()) {
                        Value *LoadedG = Builder.CreateCall(ShadowLoadFn, {Ptr}, "loaded_g");
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(LoadedG)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *Val = SI->getValueOperand();
                    Value *G = getGrade(Ptr);
                    if (!G) G = getTOP();

                    uint64_t Size = M.getDataLayout().getTypeStoreSize(Val->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});

                    if (Tier >= 1 && Val->getType()->isPointerTy()) {
                        Value *ValG = getGrade(Val);
                        if (!ValG) ValG = getTOP();
                        Builder.CreateCall(ShadowStoreFn, {Ptr, ValG});
                    }
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGrade(Ptr);
                        if (!G) G = getTOP();
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    if (!G) G = getTOP();
                    Builder.CreateCall(ProvExposeFn, {Ptr, G, Builder.getInt32(Policy)});
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *IntVal = ITP->getOperand(0);
                    if (IntVal->getType()->getIntegerBitWidth() < 64) {
                        IntVal = Builder.CreateZExt(IntVal, Builder.getInt64Ty());
                    } else if (IntVal->getType()->getIntegerBitWidth() > 64) {
                        IntVal = Builder.CreateTrunc(IntVal, Builder.getInt64Ty());
                    }

                    Value *Res = Builder.CreateCall(IntToPtrResolveFn, {IntVal, Builder.getInt32(Policy), Builder.getInt32(0)});
                    Value *NewPtr = Builder.CreateExtractValue(Res, {0}, "resolved_ptr");
                    Value *NewGrade = Builder.CreateExtractValue(Res, {1}, "resolved_grade");

                    Value *CastPtr = Builder.CreatePointerCast(NewPtr, ITP->getType());
                    ITP->replaceAllUsesWith(CastPtr);
                    if (auto *CastInst = dyn_cast<Instruction>(CastPtr)) {
                        CastInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(NewGrade)));
                    }
                    I.eraseFromParent();
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
