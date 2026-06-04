#include "GoCheckInsertPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

#define DEBUG_TYPE "go-check-insert"
STATISTIC(NumChecksInserted, "Number of safety checks inserted");
STATISTIC(NumMissingGrades, "Number of memory operations with missing grades (TOP fallback)");

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

void GoCheckInsertPass::emitRemark(Instruction *I, StringRef Message) {
    I->getContext().diagnose(OptimizationRemark("go-check-insert", "Remark", I->getDebugLoc(), I->getParent()) << Message);
}

PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

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
                    if (!G) {
                        G = getTOP(M, Builder);
                        emitRemark(LI, "Missing grade for load; using TOP");
                        NumMissingGrades++;
                    }
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(LI->getType());
                    Builder.CreateCall(CheckLoadFn, {Ptr, G, Builder.getInt64(Size)});
                    NumChecksInserted++;
                } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    Value *Ptr = SI->getPointerOperand();
                    Value *G = getGradeMetadata(Ptr);
                    if (!G) {
                        G = getTOP(M, Builder);
                        emitRemark(SI, "Missing grade for store; using TOP");
                        NumMissingGrades++;
                    }
                    uint64_t Size = M.getDataLayout().getTypeStoreSize(SI->getValueOperand()->getType());
                    Builder.CreateCall(CheckStoreFn, {Ptr, G, Builder.getInt64(Size)});
                    NumChecksInserted++;
                } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (!Callee) continue;

                    if (Callee->getName() == "free") {
                        Value *Ptr = CI->getArgOperand(0);
                        Value *G = getGradeMetadata(Ptr);
                        if (!G) {
                            G = getTOP(M, Builder);
                            emitRemark(CI, "Missing grade for free; using TOP");
                            NumMissingGrades++;
                        }
                        Builder.CreateCall(CheckFreeFn, {Ptr, G});
                        NumChecksInserted++;
                    } else if (Callee->getName().starts_with("llvm.memcpy") || Callee->getName().starts_with("llvm.memmove")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);

                        Value *GDst = nullptr;
                        if (auto *MD = CI->getMetadata("go.grade.dst"))
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();

                        Value *GSrc = nullptr;
                        if (auto *MD = CI->getMetadata("go.grade.src"))
                            GSrc = cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();

                        if (!GDst) {
                            GDst = getTOP(M, Builder);
                            emitRemark(CI, "Missing destination grade for memcpy/memmove; using TOP");
                            NumMissingGrades++;
                        }
                        if (!GSrc) {
                            GSrc = getTOP(M, Builder);
                            emitRemark(CI, "Missing source grade for memcpy/memmove; using TOP");
                            NumMissingGrades++;
                        }

                        Builder.CreateCall(CheckStoreFn, {Dst, GDst, Len});
                        Builder.CreateCall(CheckLoadFn, {Src, GSrc, Len});
                        NumChecksInserted += 2;
                    } else if (Callee->getName().starts_with("llvm.memset")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Len = CI->getArgOperand(2);

                        Value *GDst = nullptr;
                        if (auto *MD = CI->getMetadata("go.grade.dst"))
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MD)->getOperand(0))->getValue();

                        if (!GDst) {
                            GDst = getTOP(M, Builder);
                            emitRemark(CI, "Missing destination grade for memset; using TOP");
                            NumMissingGrades++;
                        }

                        Builder.CreateCall(CheckStoreFn, {Dst, GDst, Len});
                        NumChecksInserted++;
                    }
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
