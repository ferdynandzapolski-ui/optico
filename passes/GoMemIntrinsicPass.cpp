#include "GoMemIntrinsicPass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace llvm {

void GoMemIntrinsicPass::ensureTypes(Module &M) {
    if (MemcpyFn) return;
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

    MemcpyFn = M.getOrInsertFunction("__go_memcpy", Type::getVoidTy(Ctx), PtrTy, GradeTy, PtrTy, GradeTy, SizeTy, I32Ty);
    MemmoveFn = M.getOrInsertFunction("__go_memmove", Type::getVoidTy(Ctx), PtrTy, GradeTy, PtrTy, GradeTy, SizeTy, I32Ty);
    MemsetFn = M.getOrInsertFunction("__go_memset", Type::getVoidTy(Ctx), PtrTy, GradeTy, I32Ty, SizeTy);
}

PreservedAnalyses GoMemIntrinsicPass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);

    auto getTOP = [&]() {
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
                if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee && (Callee->getName().starts_with("llvm.memcpy") ||
                                   Callee->getName().starts_with("llvm.memmove"))) {
                        bool isMemmove = Callee->getName().starts_with("llvm.memmove");
                        IRBuilder<> Builder(CI);
                        Value *Dst = CI->getArgOperand(0);
                        Value *Src = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);

                        Value *GDst = nullptr;
                        Value *GSrc = nullptr;

                        if (auto *MDDst = CI->getMetadata("go.grade.dst")) {
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        }
                        if (auto *MDSrc = CI->getMetadata("go.grade.src")) {
                            GSrc = cast<ValueAsMetadata>(cast<MDNode>(MDSrc)->getOperand(0))->getValue();
                        }

                        if (!GDst) GDst = getTOP();
                        if (!GSrc) GSrc = getTOP();

                        Builder.CreateCall(isMemmove ? MemmoveFn : MemcpyFn, {Dst, GDst, Src, GSrc, Len, Builder.getInt32(0)});
                        CI->eraseFromParent();
                    } else if (Callee && Callee->getName().starts_with("llvm.memset")) {
                        IRBuilder<> Builder(CI);
                        Value *Dst = CI->getArgOperand(0);
                        Value *Val = CI->getArgOperand(1);
                        Value *Len = CI->getArgOperand(2);

                        Value *GDst = nullptr;
                        if (auto *MDDst = CI->getMetadata("go.grade.dst")) {
                            GDst = cast<ValueAsMetadata>(cast<MDNode>(MDDst)->getOperand(0))->getValue();
                        }

                        if (!GDst) GDst = getTOP();

                        if (Val->getType()->isIntegerTy(8)) {
                            Val = Builder.CreateZExt(Val, Builder.getInt32Ty());
                        }

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
