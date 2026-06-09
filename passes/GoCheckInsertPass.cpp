#include "GoCheckInsertPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

#define DEBUG_TYPE "go-check-insert"
STATISTIC(NumChecks, "Number of checks inserted");
STATISTIC(NumMissingGrades, "Number of missing grades synthesized");

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

    MemcpyFn = M.getOrInsertFunction("__go_memcpy", Type::getVoidTy(Ctx), PtrTy, GradeTy, PtrTy, GradeTy, SizeTy, I32Ty);
    MemmoveFn = M.getOrInsertFunction("__go_memmove", Type::getVoidTy(Ctx), PtrTy, GradeTy, PtrTy, GradeTy, SizeTy, I32Ty);
    MemsetFn = M.getOrInsertFunction("__go_memset", Type::getVoidTy(Ctx), PtrTy, GradeTy, Type::getInt8Ty(Ctx), SizeTy);
}

Value* GoCheckInsertPass::getTOP(Module &M, IRBuilder<> &Builder) {
    LLVMContext &Ctx = M.getContext();
    if (!GradeTy) return nullptr;
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

void GoCheckInsertPass::emitRemark(Instruction *I, StringRef Message) {
    I->getContext().diagnose(OptimizationRemark("go-check-insert", "Remark", I->getDebugLoc(), I->getParent()) << Message);
}

Value* GoCheckInsertPass::getGrade(Instruction *I, StringRef Kind) {
    if (MDNode *MD = I->getMetadata(Kind)) {
        if (MD->getNumOperands() == 2) { // !{!"go.grade", %grade}
            return cast<ValueAsMetadata>(MD->getOperand(1))->getValue();
        } else if (MD->getNumOperands() == 1) { // !{%grade}
            return cast<ValueAsMetadata>(MD->getOperand(0))->getValue();
        }
    }
    return nullptr;
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : llvm::make_early_inc_range(BB)) {
                IRBuilder<> Builder(&I);

                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    Value *Ptr = LI->getPointerOperand();
                    Value *G = getGrade(LI);
                    if (!G) {
                        G = getTOP(M, Builder);
                        NumMissingGrades++;
                        emitRemark(&I, "Missing grade for load; using TOP");
                    }
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                    NumChecks++;
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *G = getGrade(SI);
                    if (!G) {
                        G = getTOP(M, Builder);
                        NumMissingGrades++;
                        emitRemark(&I, "Missing grade for store; using TOP");
                    }
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    NumChecks++;
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (!Callee) continue;

                    if (Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGrade(CI);
                        if (!G) {
                            G = getTOP(M, Builder);
                            NumMissingGrades++;
                            emitRemark(&I, "Missing grade for free; using TOP");
                        }
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                        NumChecks++;
                    } else if (Callee->getName().starts_with("llvm.memcpy")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);
                        Value *GDst = getGrade(CI, "go.grade.dst");
                        Value *GSrc = getGrade(CI, "go.grade.src");

                        if (!GDst) GDst = getTOP(M, Builder);
                        if (!GSrc) GSrc = getTOP(M, Builder);

                        Builder.CreateCall(MemcpyFn, {Dst, GDst, Src, GSrc, Len, Builder.getInt32(0)});
                        CI->eraseFromParent();
                        NumChecks++;
                    } else if (Callee->getName().starts_with("llvm.memmove")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);
                        Value *GDst = getGrade(CI, "go.grade.dst");
                        Value *GSrc = getGrade(CI, "go.grade.src");

                        if (!GDst) GDst = getTOP(M, Builder);
                        if (!GSrc) GSrc = getTOP(M, Builder);

                        Builder.CreateCall(MemmoveFn, {Dst, GDst, Src, GSrc, Len, Builder.getInt32(0)});
                        CI->eraseFromParent();
                        NumChecks++;
                    } else if (Callee->getName().starts_with("llvm.memset")) {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *Val = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);
                        Value *G = getGrade(CI);

                        if (!G) G = getTOP(M, Builder);

                        Builder.CreateCall(MemsetFn, {Ptr, G, Val, Len});
                        CI->eraseFromParent();
                        NumChecks++;
                    }
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
