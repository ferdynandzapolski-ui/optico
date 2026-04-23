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

    ShadowStoreFn = M.getOrInsertFunction("__go_shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowLoadFn = M.getOrInsertFunction("__go_shadow_load", GradeTy, PtrTy);

    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve",
        StructType::get(Ctx, {PtrTy, GradeTy}), SizeTy, Type::getInt32Ty(Ctx));
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

    int ProvPolicy = 0; // Default: PNVI-plain
    if (auto *PolicyMD = M.getModuleFlag("go-prov-policy")) {
        ProvPolicy = cast<ConstantInt>(cast<ConstantAsMetadata>(PolicyMD)->getValue())->getZExtValue();
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
                    // Hybrid: if loading a pointer from memory, load its grade from shadow
                    if (LI->getType()->isPointerTy()) {
                        Builder.SetInsertPoint(LI->getNextNode());
                        Value *LoadedG = Builder.CreateCall(ShadowLoadFn, {Ptr}, "g_loaded");
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(LoadedG)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                    // Hybrid: if storing a pointer to memory, store its grade to shadow
                    Value *Val = SI->getValueOperand();
                    if (Val->getType()->isPointerTy()) {
                        if (Value *ValG = getGrade(Val)) {
                             Builder.CreateCall(ShadowStoreFn, {Ptr, ValG});
                        }
                    }
                } else if (auto *RMW = dyn_cast<AtomicRMWInst>(&I)) {
                    Value *Ptr = RMW->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(RMW->getValOperand()->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                } else if (auto *CXI = dyn_cast<AtomicCmpXchgInst>(&I)) {
                    Value *Ptr = CXI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(CXI->getNewValOperand()->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        Builder.CreateCall(ProvExposeFn, {Ptr, G});
                    }
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *IntVal = ITP->getOperand(0);
                    Value *Res = Builder.CreateCall(IntToPtrResolveFn, {IntVal, Builder.getInt32(ProvPolicy)}, "itp_res");
                    Value *NewPtr = Builder.CreateExtractValue(Res, {0}, "itp_ptr");
                    Value *NewG = Builder.CreateExtractValue(Res, {1}, "itp_grade");
                    ITP->replaceAllUsesWith(NewPtr);
                    if (auto *NewPtrInst = dyn_cast<Instruction>(NewPtr)) {
                        NewPtrInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(NewG)));
                    }
                    ITP->eraseFromParent();
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        if (Value *G = getGrade(Ptr)) {
                            Builder.CreateCall(CheckFreeFn, {Ptr, G});
                        }
                    }
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
