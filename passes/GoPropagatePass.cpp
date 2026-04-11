#include "GoPropagatePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/ADT/DenseMap.h"

using namespace llvm;

namespace llvm {

    void GoPropagatePass::ensureTypes(Module &M) {
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

        GepGradeFn = M.getOrInsertFunction("llvm.go.gep_grade", GradeTy, GradeTy, Type::getInt64Ty(Ctx), Type::getInt64Ty(Ctx));
        JoinGradeFn = M.getOrInsertFunction("llvm.go.join_grade", GradeTy, GradeTy, GradeTy);
    }

    Value* GoPropagatePass::getTOP(Module &M, IRBuilder<> &Builder) {
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

    void GoPropagatePass::emitRemark(Instruction *I, StringRef Message) {
        LLVMContext &Ctx = I->getContext();
        Ctx.diagnose(OptimizationRemark(
            "go-propagate", "Remark", I->getDebugLoc(), I->getParent()) << Message);
    }

    PreservedAnalyses GoPropagatePass::run(Module &M, ModuleAnalysisManager &AM) {
        ensureTypes(M);
        DenseMap<Value*, Value*> GradeMap;

        for (Function &F : M) {
            GradeMap.clear();

            // Collect existing grades from allocation sites and metadata
            for (auto &BB : F) {
                for (auto &I : BB) {
                    if (auto *MD = I.getMetadata("go.grade")) {
                        GradeMap[&I] = cast<ValueAsMetadata>(MD->getOperand(0))->getValue();
                    } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                        Function *Callee = CI->getCalledFunction();
                        if (Callee && (Callee->getName() == "llvm.go.grade_from_alloca" ||
                                       Callee->getName() == "llvm.go.grade_from_malloc")) {
                            Value *Ptr = CI->getArgOperand(0);
                            GradeMap[Ptr] = CI;
                        }
                    }
                }
            }

            bool Changed = true;
            while (Changed) {
                Changed = false;
                for (auto &BB : F) {
                    for (auto &I : BB) {
                        if (GradeMap.count(&I)) continue;
                        if (!I.getType()->isPointerTy()) continue;

                        IRBuilder<> Builder(&I);
                        // Fix for PHI: insert at first non-phi
                        if (isa<PHINode>(&I)) {
                            Builder.SetInsertPoint(BB.getFirstNonPHI());
                        } else {
                            if (auto *Next = I.getNextNode()) Builder.SetInsertPoint(Next);
                            else Builder.SetInsertPoint(&BB);
                        }

                        if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                            Value *Ptr = GEP->getPointerOperand();
                            if (GradeMap.count(Ptr)) {
                                const DataLayout &DL = M.getDataLayout();
                                APInt Offset(64, 0);
                                Value *G_p = GradeMap[Ptr];
                                Value *OffsetVal = nullptr;

                                if (GEP->accumulateConstantOffset(DL, Offset)) {
                                    OffsetVal = ConstantInt::get(Type::getInt64Ty(M.getContext()), Offset.getSExtValue());
                                    Value *G_q = Builder.CreateCall(GepGradeFn, {G_p, OffsetVal, ConstantInt::get(Type::getInt64Ty(M.getContext()), 1)}, "g_q");
                                    GEP->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_q)));
                                    GradeMap[GEP] = G_q;
                                    Changed = true;
                                } else {
                                    // Dynamic GEP fallback to TOP for now
                                    emitRemark(GEP, "Dynamic GEP; degrading to TOP grade");
                                    Value *G_top = getTOP(M, Builder);
                                    GEP->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_top)));
                                    GradeMap[GEP] = G_top;
                                    Changed = true;
                                }
                            }
                        } else if (auto *BC = dyn_cast<BitCastInst>(&I)) {
                            Value *Ptr = BC->getOperand(0);
                            if (GradeMap.count(Ptr)) {
                                Value *G_p = GradeMap[Ptr];
                                BC->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_p)));
                                GradeMap[BC] = G_p;
                                Changed = true;
                            }
                        } else if (auto *ASC = dyn_cast<AddrSpaceCastInst>(&I)) {
                            Value *Ptr = ASC->getOperand(0);
                            if (GradeMap.count(Ptr)) {
                                Value *G_p = GradeMap[Ptr];
                                ASC->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_p)));
                                GradeMap[ASC] = G_p;
                                Changed = true;
                            }
                        } else if (auto *PN = dyn_cast<PHINode>(&I)) {
                            std::vector<Value*> IncomingGrades;
                            bool AllFound = true;
                            for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
                                Value *In = PN->getIncomingValue(i);
                                if (GradeMap.count(In)) {
                                    IncomingGrades.push_back(GradeMap[In]);
                                } else {
                                    AllFound = false;
                                    break;
                                }
                            }
                            if (AllFound && !IncomingGrades.empty()) {
                                Value *G_res = IncomingGrades[0];
                                for (size_t i = 1; i < IncomingGrades.size(); ++i) {
                                    G_res = Builder.CreateCall(JoinGradeFn, {G_res, IncomingGrades[i]}, "g_phi");
                                }
                                PN->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_res)));
                                GradeMap[PN] = G_res;
                                Changed = true;
                            }
                        } else if (auto *SI = dyn_cast<SelectInst>(&I)) {
                            Value *T = SI->getTrueValue();
                            Value *F = SI->getFalseValue();
                            if (GradeMap.count(T) && GradeMap.count(F)) {
                                Value *G_sel = Builder.CreateCall(JoinGradeFn, {GradeMap[T], GradeMap[F]}, "g_sel");
                                SI->setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_sel)));
                                GradeMap[SI] = G_sel;
                                Changed = true;
                            }
                        } else {
                            // Unhandled pointer-producing opcode => TOP + remark
                            emitRemark(&I, "Unhandled pointer opcode; degrading to TOP grade");
                            Value *G_top = getTOP(M, Builder);
                            I.setMetadata("go.grade", MDNode::get(M.getContext(), ValueAsMetadata::get(G_top)));
                            GradeMap[&I] = G_top;
                            Changed = true;
                        }
                    }
                }
            }
        }

        return PreservedAnalyses::none();
    }
} // namespace llvm
