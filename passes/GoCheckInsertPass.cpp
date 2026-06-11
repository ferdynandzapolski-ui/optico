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
    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy);

    // llvm.go.inttoptr_resolve(i64) returns {ptr, go.grade}
    StructType *ResTy = StructType::get(Ctx, {PtrTy, GradeTy});
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve", ResTy, SizeTy);

    ShadowStoreFn = M.getOrInsertFunction("llvm.go.shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ShadowLoadFn = M.getOrInsertFunction("llvm.go.shadow_load", GradeTy, PtrTy);
}

Value* GoCheckInsertPass::getTOP(Module &M, IRBuilder<> &Builder) {
    LLVMContext &Ctx = M.getContext();
    GradeTy = M.getTypeByName("go.grade");
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
                    return getTOP(M, Builder);
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                    if (LI->getType()->isPointerTy()) {
                        Value *LoadedG = Builder.CreateCall(ShadowLoadFn, {Ptr});
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(LoadedG)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    Value *Val = SI->getValueOperand();
                    if (Val->getType()->isPointerTy()) {
                        Value *ValG = getGrade(Val);
                        Builder.CreateCall(ShadowStoreFn, {Ptr, ValG});
                    }
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGrade(Ptr);
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    Builder.CreateCall(ProvExposeFn, {Ptr, G});
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *Src = ITP->getOperand(0);
                    Value *Call = Builder.CreateCall(IntToPtrResolveFn, {Src});
                    Value *NewPtr = Builder.CreateExtractValue(Call, {0});
                    Value *NewGrade = Builder.CreateExtractValue(Call, {1});
                    ITP->replaceAllUsesWith(NewPtr);
                    if (auto *NewPtrInst = dyn_cast<Instruction>(NewPtr)) {
                        NewPtrInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(NewGrade)));
                    }
                    ITP->eraseFromParent();
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
