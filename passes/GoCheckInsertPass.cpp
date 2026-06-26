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

    Type *IntPtrResTy = StructType::get(Ctx, {PtrTy, GradeTy});
    IntToPtrResolveFn = M.getOrInsertFunction("llvm.go.inttoptr_resolve", IntPtrResTy, SizeTy, Type::getInt32Ty(Ctx));
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

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : llvm::make_early_inc_range(BB)) {
                IRBuilder<> Builder(&I);

                auto getGradeOrTOP = [&](Value *Ptr) -> Value* {
                    if (auto *Inst = dyn_cast<Instruction>(Ptr)) {
                        if (auto *MD = Inst->getMetadata("go.grade")) {
                            return cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();
                        }
                    }
                    return getTOP(M, Builder);
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGradeOrTOP(Ptr);
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *G = getGradeOrTOP(Ptr);
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGradeOrTOP(Ptr);
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                    }
                } else if (auto *PTI = dyn_cast<PtrToIntInst>(&I)) {
                    Value *Ptr = PTI->getPointerOperand();
                    Value *G = getGradeOrTOP(Ptr);
                    Builder.CreateCall(ProvExposeFn, {Ptr, G});
                } else if (auto *ITP = dyn_cast<IntToPtrInst>(&I)) {
                    Value *IntVal = ITP->getOperand(0);
                    // Use PNVI policy from module flag if present, default to 0 (PNVI-plain)
                    int Policy = 0;
                    if (auto *PolicyFlag = mdconst::extract_or_null<ConstantInt>(M.getModuleFlag("go-prov-policy"))) {
                        Policy = PolicyFlag->getZExtValue();
                    }

                    Value *Res = Builder.CreateCall(IntToPtrResolveFn, {IntVal, Builder.getInt32(Policy)});
                    Value *NewPtr = Builder.CreateExtractValue(Res, {0}, "p_resolved");
                    Value *NewGrade = Builder.CreateExtractValue(Res, {1}, "g_resolved");

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
