#include "GoCheckInsertPass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

namespace llvm {

    void GoCheckInsertPass::ensureTypes(Module &M) {
        if (GradeTy) return;
        LLVMContext &Ctx = M.getContext();
        GradeTy = StructType::getTypeByName(Ctx, "go.grade");
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
        std::vector<Constant*> Fields;
        Fields.push_back(ConstantInt::get(Type::getInt64Ty(Ctx), 0));
        Fields.push_back(ConstantInt::get(Type::getInt64Ty(Ctx), -1ULL));
        Fields.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), 0));
        Fields.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), 0));
        Fields.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), 0xF)); // perms
        Fields.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), 0));
        Fields.push_back(ConstantInt::get(Type::getInt64Ty(Ctx), 0));
        Fields.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), 0));
        return ConstantStruct::get(cast<StructType>(GradeTy), Fields);
    }

    void GoCheckInsertPass::emitRemark(Instruction *I, StringRef Message) {
        LLVMContext &Ctx = I->getContext();
        Ctx.diagnose(OptimizationRemark(
            "go-check-insert", "Remark", I->getDebugLoc(), I->getParent()) << Message);
    }

    PreservedAnalyses GoCheckInsertPass::run(Module &M, ModuleAnalysisManager &AM) {
        ensureTypes(M);

        for (Function &F : M) {
            for (auto &BB : F) {
                for (Instruction &I : llvm::make_early_inc_range(BB)) {
                    IRBuilder<> Builder(&I);
                    if (auto *LI = dyn_cast<LoadInst>(&I)) {
                        Value *Ptr = LI->getPointerOperand();
                        Value *Grade = nullptr;
                        if (auto *PtrInst = dyn_cast<Instruction>(Ptr)) {
                            if (auto *MD = PtrInst->getMetadata("go.grade")) {
                                Grade = cast<ValueAsMetadata>(MD->getOperand(0))->getValue();
                            }
                        }
                        if (!Grade) {
                            emitRemark(LI, "Missing grade for load; using TOP");
                            Grade = getTOP(M, Builder);
                        }
                        const DataLayout &DL = M.getDataLayout();
                        uint64_t Size = DL.getTypeStoreSize(LI->getType());
                        Builder.CreateCall(CheckLoadFn, {Ptr, Grade, ConstantInt::get(Type::getInt64Ty(M.getContext()), Size)});
                    } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                        Value *Ptr = SI->getPointerOperand();
                        Value *Grade = nullptr;
                        if (auto *PtrInst = dyn_cast<Instruction>(Ptr)) {
                            if (auto *MD = PtrInst->getMetadata("go.grade")) {
                                Grade = cast<ValueAsMetadata>(MD->getOperand(0))->getValue();
                            }
                        }
                        if (!Grade) {
                            emitRemark(SI, "Missing grade for store; using TOP");
                            Grade = getTOP(M, Builder);
                        }
                        const DataLayout &DL = M.getDataLayout();
                        uint64_t Size = DL.getTypeStoreSize(SI->getValueOperand()->getType());
                        Builder.CreateCall(CheckStoreFn, {Ptr, Grade, ConstantInt::get(Type::getInt64Ty(M.getContext()), Size)});
                    } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                        Function *Callee = CI->getCalledFunction();
                        if (Callee && Callee->getName() == "free") {
                            Value *Ptr = CI->getArgOperand(0);
                            Value *Grade = nullptr;
                            if (auto *PtrInst = dyn_cast<Instruction>(Ptr)) {
                                if (auto *MD = PtrInst->getMetadata("go.grade")) {
                                    Grade = cast<ValueAsMetadata>(MD->getOperand(0))->getValue();
                                }
                            }
                            if (!Grade) {
                                emitRemark(CI, "Missing grade for free; using TOP");
                                Grade = getTOP(M, Builder);
                            }
                            Builder.CreateCall(CheckFreeFn, {Ptr, Grade});
                        }
                    }
                }
            }
        }

        return PreservedAnalyses::none();
    }
} // namespace llvm
