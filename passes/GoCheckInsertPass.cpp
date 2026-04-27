#include "GoCheckInsertPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

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

    PtrGradeTy = M.getTypeByName("go.ptr_grade");
    if (!PtrGradeTy) {
        PtrGradeTy = StructType::create(Ctx, {
            PointerType::getUnqual(Ctx), GradeTy
        }, "go.ptr_grade");
    }

    Type *PtrTy = PointerType::getUnqual(Ctx);
    Type *SizeTy = Type::getInt64Ty(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);
    Type *IntPtrTy = M.getDataLayout().getIntPtrType(Ctx);

    CheckLoadFn = M.getOrInsertFunction("llvm.go.check_load", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckStoreFn = M.getOrInsertFunction("llvm.go.check_store", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckFreeFn = M.getOrInsertFunction("llvm.go.check_free", Type::getVoidTy(Ctx), PtrTy, GradeTy);

    ShadowStoreFn = M.getOrInsertFunction("llvm.go.shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowLoadFn = M.getOrInsertFunction("llvm.go.shadow_load", GradeTy, PtrTy);

    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve", PtrGradeTy, IntPtrTy, I32Ty);
}

Value* GoCheckInsertPass::getTOP(Module &M, IRBuilder<> &Builder) {
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
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

    int Tier = 0;
    if (auto *TierFlag = mdconst::extract_or_null<ConstantInt>(M.getModuleFlag("go-tier"))) {
        Tier = TierFlag->getZExtValue();
    }

    int ProvPolicy = 0;
    if (auto *PolicyFlag = mdconst::extract_or_null<ConstantInt>(M.getModuleFlag("go-prov-policy"))) {
        ProvPolicy = PolicyFlag->getZExtValue();
    }

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : llvm::make_early_inc_range(BB)) {
                if (I.getFunction() == nullptr) continue; // Skip if already erased
                IRBuilder<> Builder(&I);

                auto getGrade = [&](Value *Ptr) -> Value* {
                    if (auto *Inst = dyn_cast<Instruction>(Ptr)) {
                        if (auto *MD = Inst->getMetadata("go.grade")) {
                            return cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();
                        }
                    }
                    if (auto *Arg = dyn_cast<Argument>(Ptr)) {
                         // Arguments don't have metadata directly usually, but in a real pass we'd track them.
                         // For now, return null to fallback to TOP.
                    }
                    return nullptr;
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    if (!G) G = getTOP(M, Builder);

                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});

                    // If loading a pointer, and in hybrid tier, load its grade from shadow
                    if (Tier >= 1 && LI->getType()->isPointerTy()) {
                        Builder.SetInsertPoint(LI->getNextNode());
                        Value *ShadowG = Builder.CreateCall(ShadowLoadFn, {Ptr}, "g_load");
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(ShadowG)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    if (!G) G = getTOP(M, Builder);

                    uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});

                    // If storing a pointer, and in hybrid tier, store its grade to shadow
                    if (Tier >= 1 && SI->getValueOperand()->getType()->isPointerTy()) {
                        Value *ValG = getGrade(SI->getValueOperand());
                        if (!ValG) ValG = getTOP(M, Builder);
                        Builder.CreateCall(ShadowStoreFn, {Ptr, ValG});
                    }
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGrade(Ptr);
                        if (!G) G = getTOP(M, Builder);
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    if (!G) G = getTOP(M, Builder);
                    Builder.CreateCall(ProvExposeFn, {Ptr, G});
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *IntVal = ITP->getOperand(0);
                    Value *Res = Builder.CreateCall(IntToPtrResolveFn, {IntVal, Builder.getInt32(ProvPolicy)}, "g_res");
                    Value *NewPtr = Builder.CreateExtractValue(Res, {0}, "p_res");
                    Value *NewGrade = Builder.CreateExtractValue(Res, {1}, "g_res_val");

                    if (auto *NewPtrInst = dyn_cast<Instruction>(NewPtr)) {
                         NewPtrInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(NewGrade)));
                    }
                    ITP->replaceAllUsesWith(NewPtr);
                    ITP->eraseFromParent();
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
