#include "TypeRecovery/SSGSolver.h"
#include "TypeRecovery/StorageShapeGraph.h"
#include "Passes/ConstraintGenerator.h"

namespace notdec::retypd {

PreservedAnalyses SSGTypeRec::run(Module &M, ModuleAnalysisManager &) {
  SSGVisitor visitor(*this);
  for (Function &F : M) {
    visitor.visit(F);
  }
  SSG.solve();
  // merge inter-procedural
  return PreservedAnalyses::all();
}

SSGNode &SSGTypeRec::getOrInsertNode(llvm::Value *Val, User *User) {
  return getOrInsertNode(Val, Val->getType()->getScalarSizeInBits(), User);
}

SSGNode &SSGTypeRec::getOrInsertNode(ExtValPtr Key, unsigned int Size,
                                     User *User) {
  // Differentiate int32/int64 by User.
  if (auto V = std::get_if<llvm::Value *>(&Key)) {
    if (auto CI = dyn_cast<Constant>(*V)) {
        assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
        assert(hasUser(*V, User) &&
               "convertTypeVarVal: constant not used by user");
        Key = UConstant{.Val = cast<Constant>(*V), .User = User};
    }
  }
  auto [It, Inserted] = Val2Node.try_emplace(Key, SSG);
  if (Inserted) {
    // Initially, every node points to a new unknown node
    auto *NewNode = SSG.createUnknown(Size);
    NewNode->setName(getName(Key));
    It->second.setNode(NewNode);
    if (auto *Val = std::get_if<llvm::Value *>(&Key)) {
      if (mustBePrimitive((*Val)->getType())) {
        It->second.lookupNode()->getPNVar()->setPtrOrNum(retypd::NonPtr);
      }
    } else if (auto *IntConst = std::get_if<IntConstant>(&Key)) {
      // set as int constant, so that it will be mounted to memory if it becomes
      // a pointer
      It->second.lookupNode()->setIntConstant(IntConst->Val);
    }
  }
  return *It->second.lookupNode();
}

void SSGTypeRec::setAsPtr(SSGNode &Node) {
  // set as pointer type
  Node.getPNVar()->setPtrOrNum(retypd::Pointer);
}
void SSGTypeRec::setAsNonPtr(SSGNode &Node) {
  // set as pointer type
  Node.getPNVar()->setPtrOrNum(retypd::NonPtr);
}

void SSGTypeRec::addSubTypeCons(SSGNode &DestNode, SSGNode &SrcNode,
                                OffsetRange Offset) {
  DestNode.getPNVar()->unifyPN(*SrcNode.getPNVar());
  SSG.addSubTypeCons(&DestNode, &SrcNode, Offset);
}

void SSGVisitor::visitAllocaInst(AllocaInst &I) {
  const char *prefix = "alloca_";
  if (I.getParent()->isEntryBlock()) {
    prefix = "stack_";
  }
  ValueNamer::getName(I, prefix);
  Ctx.setAsPtr(Ctx.getOrInsertNode(&I));
}

void SSGVisitor::visitCastInst(CastInst &I) {
  if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto &Node = Ctx.getOrInsertNode(&I);
    auto &SrcNode = Ctx.getOrInsertNode(I.getOperand(0));
    Ctx.addSubTypeCons(Node, SrcNode);
    return;
  } else if (isa<TruncInst, ZExtInst>(&I)) {
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: RetypdGenerator::getTypeVar unhandled CastInst: " << I
               << "\n";
  std::abort();
}

void SSGVisitor::visitAdd(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  Ctx.SSG.addAddCons(&Ctx.getOrInsertNode(Src1, &I),
                     &Ctx.getOrInsertNode(Src2, &I), &Ctx.getOrInsertNode(&I),
                     &I);
}

void SSGVisitor::visitSub(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  Ctx.SSG.addSubCons(&Ctx.getOrInsertNode(Src1), &Ctx.getOrInsertNode(Src2),
                     &Ctx.getOrInsertNode(&I), &I);
}

void SSGVisitor::visitLoadInst(LoadInst &I) {
  auto Addr = I.getPointerOperand();
  Ctx.setAsPtr(Ctx.getOrInsertNode(Addr, &I));
  auto &AddrNode = Ctx.getOrInsertNode(Addr);
  auto &Node = Ctx.getOrInsertNode(&I);
  AddrNode.delegateToOffset(Node, 0);
}

void SSGVisitor::visitStoreInst(StoreInst &I) {
  auto Addr = I.getPointerOperand();
  Ctx.setAsPtr(Ctx.getOrInsertNode(Addr, &I));
  auto &AddrNode = Ctx.getOrInsertNode(Addr);
  auto &Node = Ctx.getOrInsertNode(I.getOperand(0));
  AddrNode.delegateToOffset(Node, 0);
}

void SSGVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: RetypdGenerator: indirect call not supported yet\n";
    return;
  }
  for (int i = 0; i < I.arg_size(); i++) {
    auto &ArgVar = Ctx.getOrInsertNode(
        CallArg{.Call = &I, .InstanceId = 0, .Index = i},
        I.getArgOperand(i)->getType()->getScalarSizeInBits());
    auto &ValVar = Ctx.getOrInsertNode(I.getArgOperand(i), &I);
    // argument is a subtype of param
    Ctx.addSubTypeCons(ValVar, ArgVar);
  }
  if (!I.getType()->isVoidTy()) {
    // for return value
    auto DstVar = Ctx.getOrInsertNode(CallRet{.Call = &I},
                                      I.getType()->getScalarSizeInBits());
    Ctx.addSubTypeCons(Ctx.getOrInsertNode(&I), DstVar);
  }
}

void SSGVisitor::visitInstruction(Instruction &I) {
  // return value
  if (I.getType()->isVoidTy()) {
    // skip void type
  } else if (mustBePrimitive(I.getType())) {
    Ctx.setAsNonPtr(Ctx.getOrInsertNode(&I));
  } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
             ConstraintsGenerator::opTypes.at(I.getOpcode()).output !=
                 nullptr) {
    Ctx.setAsNonPtr(Ctx.getOrInsertNode(&I));
  } else {
    llvm::errs() << "WARN: RetypdGenerator: unhandled instruction return: " << I
                 << "\n";
  }
  // for each op
  for (unsigned Ind = 0; Ind < I.getNumOperands(); ++Ind) {
    auto Op = I.getOperand(Ind);
    if (Op->getType()->isVoidTy()) {
      // skip void type
    } else if (mustBePrimitive(Op->getType())) {
      Ctx.setAsNonPtr(Ctx.getOrInsertNode(Op, &I));
    } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
               ConstraintsGenerator::opTypes.at(I.getOpcode()).inputs[Ind] !=
                   nullptr) {
      Ctx.setAsNonPtr(Ctx.getOrInsertNode(Op, &I));
    } else {
      llvm::errs() << "WARN: RetypdGenerator: unhandled instruction Op: " << I
                   << "\n";
    }
  }
}

} // namespace notdec::retypd
