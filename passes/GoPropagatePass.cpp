#include "GoPropagatePass.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

namespace llvm {

void GoPropagatePass::ensureTypes(Module &M) {
    if (GradeTy) return;
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
    Type *I64Ty = Type::getInt64Ty(Ctx);
    GepGradeFn = M.getOrInsertFunction("llvm.go.gep_grade", GradeTy, GradeTy, I64Ty, I64Ty);
    JoinGradeFn = M.getOrInsertFunction("llvm.go.join_grade", GradeTy, GradeTy, GradeTy);
}

Value* GoPropagatePass::getTOP(Module &M, IRBuilder<> &Builder) {
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

void GoPropagatePass::emitRemark(Instruction *I, StringRef Message) {
    I->getContext().diagnose(OptimizationRemark("go-propagate", "Remark", I->getDebugLoc(), I->getParent()) << Message);
}

PreservedAnalyses GoPropagatePass::run(Module &M, ModuleAnalysisManager &AM) {
    ensureTypes(M);
    std::map<Value*, Value*> GradeMap;

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (auto *CI = dyn_cast<CallInst>(&I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (!Callee) continue;
                    if (Callee->getName() == "llvm.go.grade_from_alloca" ||
                        Callee->getName() == "llvm.go.grade_from_malloc") {
                        Value *Ptr = CI->getArgOperand(0);
                        GradeMap[Ptr] = CI;
                        if (auto *PtrInst = dyn_cast<Instruction>(Ptr)) {
                             PtrInst->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(CI)));
                        }
                    }
                }
            }
        }
    }

    bool Changed = true;
    while (Changed) {
        Changed = false;
        for (Function &F : M) {
            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (auto *CI = dyn_cast<CallInst>(&I)) {
                        Function *Callee = CI->getCalledFunction();
                        if (Callee && (Callee->getName().starts_with("llvm.memcpy") ||
                                       Callee->getName().starts_with("llvm.memmove"))) {
                            Value *Dst = CI->getArgOperand(0);
                            Value *Src = CI->getArgOperand(1);
                            if (GradeMap.count(Dst) && !CI->getMetadata("go.grade.dst")) {
                                CI->setMetadata("go.grade.dst", MDNode::get(M.getContext(), ValueAsMetadata::get(GradeMap[Dst])));
                                Changed = true;
                            }
                            if (GradeMap.count(Src) && !CI->getMetadata("go.grade.src")) {
                                CI->setMetadata("go.grade.src", MDNode::get(M.getContext(), ValueAsMetadata::get(GradeMap[Src])));
                                Changed = true;
                            }
                        } else if (Callee && Callee->getName().starts_with("llvm.memset")) {
                            Value *Dst = CI->getArgOperand(0);
                            if (GradeMap.count(Dst) && !CI->getMetadata("go.grade")) {
                                CI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(GradeMap[Dst])));
                                Changed = true;
                            }
                        }
                    }

                    if (GradeMap.count(&I)) continue;
                    if (!I.getType()->isPointerTy()) continue;

                    IRBuilder<> Builder(&I);
                    Value *G = nullptr;

                    if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                        Value *BasePtr = GEP->getPointerOperand();
                        if (GradeMap.count(BasePtr)) {
                            APInt Offset(64, 0);
                            if (GEP->accumulateConstantOffset(M.getDataLayout(), Offset)) {
                                G = Builder.CreateCall(GepGradeFn, {GradeMap[BasePtr], Builder.getInt64(Offset.getSExtValue()), Builder.getInt64(1)}, "g_gep");
                            } else {
                                G = getTOP(M, Builder);
                                emitRemark(GEP, "Dynamic GEP; using TOP grade");
                            }
                        }
                    } else if (auto *BC = dyn_cast<BitCastInst>(&I)) {
                        if (GradeMap.count(BC->getOperand(0))) G = GradeMap[BC->getOperand(0)];
                    } else if (auto *ASC = dyn_cast<AddrSpaceCastInst>(&I)) {
                        if (GradeMap.count(ASC->getOperand(0))) G = GradeMap[ASC->getOperand(0)];
                    } else if (auto *PN = dyn_cast<PHINode>(&I)) {
                        bool AllHaveGrades = true;
                        for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
                            if (!GradeMap.count(PN->getIncomingValue(i))) { AllHaveGrades = false; break; }
                        }
                        if (AllHaveGrades) {
                            PHINode *GPhi = PHINode::Create(GradeTy, PN->getNumIncomingValues(), "g_phi_node", PN);
                            for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
                                GPhi->addIncoming(GradeMap[PN->getIncomingValue(i)], PN->getIncomingBlock(i));
                            }
                            G = GPhi;
                        }
                    } else if (auto *SI = dyn_cast<SelectInst>(&I)) {
                        if (GradeMap.count(SI->getTrueValue()) && GradeMap.count(SI->getFalseValue())) {
                            G = Builder.CreateCall(JoinGradeFn, {GradeMap[SI->getTrueValue()], GradeMap[SI->getFalseValue()]}, "g_sel");
                        }
                    }

                    if (G) {
                        GradeMap[&I] = G;
                        I.setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G)));
                        Changed = true;
                    }
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

} // namespace llvm
