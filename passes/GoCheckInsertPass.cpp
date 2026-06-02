#include "GoCheckInsertPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/ADT/Statistic.h"

using namespace llvm;

#define DEBUG_TYPE "go-check-insert"
STATISTIC(NumMissingGrades, "Number of memory operations with missing grades");

namespace llvm {

void GoCheckInsertPass::ensureTypes(Module &M) {
    if (CheckLoadFn) return;
    LLVMContext &Ctx = M.getContext();
    GradeTy = M.getTypeByName("go.grade");
    if (!GradeTy) {
        GradeTy = StructType::create(Ctx, {
            Type::getInt64Ty(Ctx), // base
            Type::getInt64Ty(Ctx), // end
            Type::getInt32Ty(Ctx), // alloc_id
            Type::getInt32Ty(Ctx), // epoch
            Type::getInt32Ty(Ctx), // perms
            Type::getInt32Ty(Ctx), // prov_tag
            Type::getInt64Ty(Ctx), // alias_tok
            Type::getInt32Ty(Ctx)  // flags
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
    I->getContext().diagnose(OptimizationRemark(DEBUG_TYPE, "Remark", I->getDebugLoc(), I->getParent()) << Message);
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
                    NumMissingGrades++;
                    emitRemark(&I, "Missing grade for pointer; using TOP grade");
                    return getTOP(M, Builder);
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

                        Value *GDst = nullptr;
                        if (auto *MDDst = CI->getMetadata("go.grade.dst")) {
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        } else {
                            NumMissingGrades++;
                            emitRemark(CI, "Missing destination grade for memcpy; using TOP grade");
                            GDst = getTOP(M, Builder);
                        }

                        Value *GSrc = nullptr;
                        if (auto *MDSrc = CI->getMetadata("go.grade.src")) {
                            GSrc = cast<ValueAsMetadata>(cast<MDNode>(MDSrc)->getOperand(0))->getValue();
                        } else {
                            NumMissingGrades++;
                            emitRemark(CI, "Missing source grade for memcpy; using TOP grade");
                            GSrc = getTOP(M, Builder);
                        }

                        Builder.CreateCall(CheckStoreFn, {Dst, GDst, Len});
                        Builder.CreateCall(CheckLoadFn, {Src, GSrc, Len});
                    } else if (Callee->getName().starts_with("llvm.memmove")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);

                        // Same as memcpy for now (coarse checks)
                        Value *GDst = nullptr;
                        if (auto *MDDst = CI->getMetadata("go.grade.dst")) {
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        } else {
                            NumMissingGrades++;
                            GDst = getTOP(M, Builder);
                        }

                        Value *GSrc = nullptr;
                        if (auto *MDSrc = CI->getMetadata("go.grade.src")) {
                            GSrc = cast<ValueAsMetadata>(cast<MDNode>(MDSrc)->getOperand(0))->getValue();
                        } else {
                            NumMissingGrades++;
                            GSrc = getTOP(M, Builder);
                        }

                        Builder.CreateCall(CheckStoreFn, {Dst, GDst, Len});
                        Builder.CreateCall(CheckLoadFn, {Src, GSrc, Len});
                    } else if (Callee->getName().starts_with("llvm.memset")) {
                        Value *Dst = CI->getArgOperand(0);
                        Value *Len = CI->getArgOperand(2);

                        Value *GDst = nullptr;
                        if (auto *MDDst = CI->getMetadata("go.grade.dst")) {
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        } else {
                            NumMissingGrades++;
                            GDst = getTOP(M, Builder);
                        }
                        Builder.CreateCall(CheckStoreFn, {Dst, GDst, Len});
                    }
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
