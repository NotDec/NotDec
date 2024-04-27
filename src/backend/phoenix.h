#ifndef _NOTDEC_BACKEND_PHOENIX_H_
#define _NOTDEC_BACKEND_PHOENIX_H_

#include "backend/CFG.h"
#include "backend/Dominators.h"
#include "backend/structural-analysis.h"
#include <utility>
#include <vector>

namespace notdec::backend {

class Phoenix : IStructuralAnalysis {
public:
  enum LoopType {
    While,
    DoWhile,
  };
  enum VirtualEdgeType {
    Goto,
    Break,
    Continue,
  };
  struct VirtualEdge {
    CFGBlock *From;
    CFGBlock *To;
    VirtualEdgeType Ty;
    VirtualEdge(CFGBlock *From, CFGBlock *To, VirtualEdgeType Ty)
        : From(From), To(To), Ty(Ty) {}
  };

protected:
  bool isCanceled = false;
  CFGDomTree Dom;
  std::vector<CFGBlock *> unresolvedSwitches;
  std::vector<std::pair<CFGBlock *, std::set<CFGBlock *>>> unresolvedCycles;
  bool ReduceAcyclic(CFGBlock *Block);
  bool isCyclic(CFGBlock *Block);
  bool ReduceCyclic(CFGBlock *Block);
  bool reduceSequence(CFGBlock *Block);
  bool reduceIfRegion(CFGBlock *Block);
  bool reduceSelf(CFGBlock *Block);
  bool ProcessUnresolvedRegions();
  bool refineLoop(CFGBlock *head, std::set<CFGBlock *> &loopNodes);
  void refineIncSwitch(CFGBlock *switchHead);
  bool virtualizeReturn(CFGBlock *B);
  bool coalesceTailRegion(CFGBlock *n, std::set<CFGBlock *> &lexicalNodes);
  bool lastResort(CFGBlock *B);
  bool lastResort(std::set<CFGBlock *> &n);
  std::set<CFGBlock *> getLexicalNodes(CFGBlock *head, CFGBlock *follow,
                                       std::set<CFGBlock *> &loopNodes);
  bool isBackEdge(CFGBlock *A, CFGBlock *B);
  std::pair<CFGBlock *, CFGBlock *>
  determineFollowLatch(CFGBlock *head, std::set<CFGBlock *> &loopNodes);
  void virtualizeEdge(VirtualEdge &edge);
  void collapseToTailRegion(CFGBlock *From, CFGBlock *To, clang::Stmt *stm);
  bool virtualizeIrregularExits(CFGBlock *head, CFGBlock *latch,
                                CFGBlock *follow,
                                std::set<CFGBlock *> &lexicalNodes);
  llvm::Optional<Phoenix::VirtualEdge>
  findLastResortEdge(std::set<CFGBlock *> &blocks);

public:
  Phoenix(SAFuncContext &Ctx) : IStructuralAnalysis(Ctx) {}

  void execute() override;
};

} // namespace notdec::backend

#endif
