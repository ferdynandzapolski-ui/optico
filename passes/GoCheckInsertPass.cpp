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
    Type *I32Ty = Type::getInt32Ty(Ctx);

    CheckLoadFn = M.getOrInsertFunction("llvm.go.check_load", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckStoreFn = M.getOrInsertFunction("llvm.go.check_store", Type::getVoidTy(Ctx), PtrTy, GradeTy, SizeTy);
    CheckFreeFn = M.getOrInsertFunction("llvm.go.check_free", Type::getVoidTy(Ctx), PtrTy, GradeTy);

    ShadowStoreFn = M.getOrInsertFunction("llvm.go.shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowLoadFn = M.getOrInsertFunction("llvm.go.shadow_load", GradeTy, PtrTy);

    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve",
                                              StructType::get(Ctx, {PtrTy, GradeTy}),
                                              SizeTy, I32Ty);
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

    int Tier = 0; // Default to diag
    if (auto *TierMD = mdconst::dyn_extract_or_null<ConstantInt>(M.getModuleFlag("go-tier"))) {
        Tier = TierMD->getZExtValue();
    }

    int ProvPolicy = 0; // PNVI-plain by default
    if (auto *PolicyMD = mdconst::dyn_extract_or_null<ConstantInt>(M.getModuleFlag("go-prov-policy"))) {
        ProvPolicy = PolicyMD->getZExtValue();
    }

    auto getTOP = [&](IRBuilder<> &Builder) {
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

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : llvm::make_early_inc_range(BB)) {
                IRBuilder<> Builder(&I);

                auto getGradeMetadata = [&](Value *Ptr) -> Value* {
                    if (auto *Inst = dyn_cast<Instruction>(Ptr)) {
                        if (auto *MD = Inst->getMetadata("go.grade")) {
                            return cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();
                        }
                    }
                    return nullptr;
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGradeMetadata(Ptr);

                    if (G) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                        Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                    }

                    if (Tier == 1 && LI->getType()->isPointerTy()) {
                        IRBuilder<> PostBuilder(LI->getNextNode() ? LI->getNextNode() : (Instruction*)nullptr);
                        if (!LI->getNextNode()) PostBuilder.SetInsertPoint(LI->getParent());
                        else PostBuilder.SetInsertPoint(LI->getNextNode());

                        Value *ShadowG = PostBuilder.CreateCall(ShadowLoadFn, {Ptr}, "shadow_g");
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(ShadowG)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *Val = SI->getValueOperand();
                    Value *G = getGradeMetadata(Ptr);

                    if (G) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(Val->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    }

                    if (Tier == 1 && Val->getType()->isPointerTy()) {
                        Value *ValG = getGradeMetadata(Val);
                        if (!ValG) ValG = getTOP(Builder);
                        Builder.CreateCall(ShadowStoreFn, {Ptr, ValG});
                    }
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        if (Value *G = getGradeMetadata(Ptr)) {
                            Builder.CreateCall(CheckFreeFn, {Ptr, G});
                        }
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    if (Value *G = getGradeMetadata(Ptr)) {
                        Builder.CreateCall(ProvExposeFn, {Ptr, G});
                    }
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *IntVal = ITP->getOperand(0);
                    Value *Res = Builder.CreateCall(IntToPtrResolveFn, {IntVal, Builder.getInt32(ProvPolicy)}, "res");
                    Value *NewPtr = Builder.CreateExtractValue(Res, {0}, "new_ptr");
                    Value *NewG = Builder.CreateExtractValue(Res, {1}, "new_g");

                    ITP->replaceAllUsesWith(NewPtr);
                    if (auto *NewPtrInst = dyn_cast<Instruction>(NewPtr)) {
                        NewPtrInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(NewG)));
                    }
                    // We can't erase ITP yet because we are iterating. It will be cleaned up later or by DCE.
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
