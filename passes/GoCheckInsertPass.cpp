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

    ShadowLoadFn = M.getOrInsertFunction("llvm.go.shadow_load", GradeTy, PtrTy);
    ShadowStoreFn = M.getOrInsertFunction("llvm.go.shadow_store", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    ProvExposeFn = M.getOrInsertFunction("llvm.go.prov_expose", Type::getVoidTy(Ctx), PtrTy, GradeTy);
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve",
                                              StructType::get(Ctx, {PtrTy, GradeTy}),
                                              SizeTy);
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
                    } else if (auto *Arg = dyn_cast<Argument>(Ptr)) {
                        // For now, arguments don't have grades unless we propagate them via metadata or similar.
                        // In a real implementation, we might have a map or attributes.
                    }
                    return nullptr;
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                        Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                    if (LI->getType()->isPointerTy()) {
                        Value *LoadedG = Builder.CreateCall(ShadowLoadFn, {Ptr}, "loaded_g");
                        LI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(LoadedG)));
                    }
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *Val = SI->getValueOperand();
                    if (Value *G = getGrade(Ptr)) {
                        uint64_t Size = M.getDataLayout().getTypeStoreSize(Val->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    }
                    if (Val->getType()->isPointerTy()) {
                        Value *StoredG = getGrade(Val);
                        if (!StoredG) {
                            // Fallback to TOP
                            LLVMContext &Ctx = M.getContext();
                            StoredG = ConstantStruct::get(cast<StructType>(GradeTy), {
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
                        Builder.CreateCall(ShadowStoreFn, {Ptr, StoredG});
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    if (Value *G = getGrade(Ptr)) {
                        Builder.CreateCall(ProvExposeFn, {Ptr, G});
                    }
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *IntVal = ITP->getOperand(0);
                    Type *I64Ty = Type::getInt64Ty(M.getContext());
                    if (IntVal->getType()->getScalarSizeInBits() < 64)
                        IntVal = Builder.CreateZExt(IntVal, I64Ty);
                    else if (IntVal->getType()->getScalarSizeInBits() > 64)
                        IntVal = Builder.CreateTrunc(IntVal, I64Ty);
                    Value *Res = Builder.CreateCall(IntToPtrResolveFn, {IntVal}, "resolved_ptr_g");
                    Value *NewPtr = Builder.CreateExtractValue(Res, {0}, "ptr");
                    Value *NewG = Builder.CreateExtractValue(Res, {1}, "g");

                    Value *CastPtr = Builder.CreateBitCast(NewPtr, ITP->getType());
                    ITP->replaceAllUsesWith(CastPtr);
                    if (auto *CastInst = dyn_cast<Instruction>(CastPtr)) {
                        CastInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(NewG)));
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
