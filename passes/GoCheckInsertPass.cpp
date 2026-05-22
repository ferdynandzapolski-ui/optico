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

    Type *PtrTy = PointerType::getUnqual(Ctx);
    Type *SizeTy = Type::getInt64Ty(Ctx);

    CheckLoadFn = M.getOrInsertFunction("llvm.go.check_load", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckStoreFn = M.getOrInsertFunction("llvm.go.check_store", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckFreeFn = M.getOrInsertFunction("llvm.go.check_free", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowStoreFn = M.getOrInsertFunction("llvm.go.shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowLoadFn = M.getOrInsertFunction("llvm.go.shadow_load", GradeTy, PtrTy);
    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve", GradeTy, Type::getInt64Ty(Ctx), Type::getInt32Ty(Ctx));
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);
    int Tier = 0;
    if (auto *F = M.getModuleFlag("go-tier")) {
        Tier = mdconst::extract<ConstantInt>(F)->getZExtValue();
    }
    int ProvPolicy = 0;
    if (auto *F = M.getModuleFlag("go-prov-policy")) {
        ProvPolicy = mdconst::extract<ConstantInt>(F)->getZExtValue();
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

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                        Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                    if (Tier == 1 && LI->getType()->isPointerTy()) {
                        Value *GLoaded = Builder.CreateCall(ShadowLoadFn, {Ptr}, "g_loaded");
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(GLoaded)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                    if (Tier == 1 && SI->getValueOperand()->getType()->isPointerTy()) {
                        if (Value *GStore = getGrade(SI->getValueOperand())) {
                            Builder.CreateCall(ShadowStoreFn, {Ptr, GStore});
                        }
                    }
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        if (Value *G = getGrade(Ptr)) {
                            Builder.CreateCall(CheckFreeFn, {Ptr, G});
                        }
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    if (Value *G = getGrade(PTI->getPointerOperand())) {
                        Builder.CreateCall(ProvExposeFn, {PTI->getPointerOperand(), G});
                    }
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *G = Builder.CreateCall(IntToPtrResolveFn, {ITP->getOperand(0), Builder.getInt32(ProvPolicy)}, "g_resolved");
                    ITP->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G)));
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
