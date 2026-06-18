#include "GoCheckInsertPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

namespace llvm {

void GoCheckInsertPass::emitRemark(Instruction *I, StringRef Message) {
    I->getContext().diagnose(OptimizationRemark("go-check-insert", "Remark", I->getDebugLoc(), I->getParent()) << Message);
}

Value* GoCheckInsertPass::getTOPGrade(Module &M) {
    LLVMContext &Ctx = M.getContext();
    ensureTypes(M);
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

    Type *I32Ty = Type::getInt32Ty(Ctx);
    MemcpyFn = M.getOrInsertFunction("__go_memcpy", Type::getVoidTy(Ctx), PtrTy, GradeTy, PtrTy, GradeTy, SizeTy, I32Ty);
    MemmoveFn = M.getOrInsertFunction("__go_memmove", Type::getVoidTy(Ctx), PtrTy, GradeTy, PtrTy, GradeTy, SizeTy, I32Ty);
    MemsetFn = M.getOrInsertFunction("__go_memset", Type::getVoidTy(Ctx), PtrTy, GradeTy, Type::getInt8Ty(Ctx), SizeTy);
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
                    // Handle missing grade by synthesizing TOP
                    emitRemark(&I, "Missing grade for pointer; synthesizing TOP grade");
                    return getTOPGrade(M);
                };

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *G = getGrade(Ptr);
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (!Callee) continue;
                    if (Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGrade(Ptr);
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                    } else if (Callee->getName().starts_with("llvm.memcpy")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);
                        Value *GDst = getTOPGrade(M);
                        Value *GSrc = getTOPGrade(M);
                        if (auto *MDDst = CI->getMetadata("go.grade.dst"))
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        if (auto *MDSrc = CI->getMetadata("go.grade.src"))
                            GSrc = cast<ValueAsMetadata>(cast<MDNode>(MDSrc)->getOperand(0))->getValue();
                        Builder.CreateCall(MemcpyFn, {Dst, GDst, Src, GSrc, Len, Builder.getInt32(0)});
                        CI->eraseFromParent();
                    } else if (Callee->getName().starts_with("llvm.memmove")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);
                        Value *GDst = getTOPGrade(M);
                        Value *GSrc = getTOPGrade(M);
                        if (auto *MDDst = CI->getMetadata("go.grade.dst"))
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        if (auto *MDSrc = CI->getMetadata("go.grade.src"))
                            GSrc = cast<ValueAsMetadata>(cast<MDNode>(MDSrc)->getOperand(0))->getValue();
                        Builder.CreateCall(MemmoveFn, {Dst, GDst, Src, GSrc, Len, Builder.getInt32(0)});
                        CI->eraseFromParent();
                    } else if (Callee->getName().starts_with("llvm.memset")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Val = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);
                        Value *GDst = getTOPGrade(M);
                        if (auto *MDDst = CI->getMetadata("go.grade"))
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        Builder.CreateCall(MemsetFn, {Dst, GDst, Val, Len});
                        CI->eraseFromParent();
                    }
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
