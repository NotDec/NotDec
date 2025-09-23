#ifndef _NOTDEC_UTILS_CG_DOT_H_
#define _NOTDEC_UTILS_CG_DOT_H_

#include "llvm/Support/DOTGraphTraits.h"
#include <llvm/ADT/SmallSet.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/HeatUtils.h>

namespace notdec::utils {

const bool CallMultiGraph = false;
const bool ShowEdgeWeight = true;
const bool ShowHeatColors = true;
const bool HideExternalFuncs = true;

class CallGraphDOTInfo {
private:
  llvm::Module *M;
  llvm::CallGraph *CG;
  llvm::DenseMap<const llvm::Function *, uint64_t> Freq;
  uint64_t MaxFreq;

public:
  std::function<llvm::BlockFrequencyInfo *(llvm::Function &)> LookupBFI;

  CallGraphDOTInfo(
      llvm::Module *M, llvm::CallGraph *CG,
      llvm::function_ref<llvm::BlockFrequencyInfo *(llvm::Function &)>
          LookupBFI)
      : M(M), CG(CG), LookupBFI(LookupBFI) {
    MaxFreq = 0;

    for (llvm::Function &F : M->getFunctionList()) {
      uint64_t localSumFreq = 0;
      llvm::SmallSet<llvm::Function *, 16> Callers;
      for (llvm::User *U : F.users())
        if (llvm::isa<llvm::CallInst>(U))
          Callers.insert(llvm::cast<llvm::Instruction>(U)->getFunction());
      for (llvm::Function *Caller : Callers)
        localSumFreq += llvm::getNumOfCalls(*Caller, F);
      if (localSumFreq >= MaxFreq)
        MaxFreq = localSumFreq;
      Freq[&F] = localSumFreq;
    }
    if (!CallMultiGraph)
      removeParallelEdges();
  }

  llvm::Module *getModule() const { return M; }

  llvm::CallGraph *getCallGraph() const { return CG; }

  uint64_t getFreq(const llvm::Function *F) { return Freq[F]; }

  uint64_t getMaxFreq() { return MaxFreq; }

private:
  void removeParallelEdges() {
    for (auto &I : (*CG)) {
      llvm::CallGraphNode *Node = I.second.get();

      bool FoundParallelEdge = true;
      while (FoundParallelEdge) {
        llvm::SmallSet<llvm::Function *, 16> Visited;
        FoundParallelEdge = false;
        for (auto CI = Node->begin(), CE = Node->end(); CI != CE; CI++) {
          if (!(Visited.insert(CI->second->getFunction())).second) {
            FoundParallelEdge = true;
            Node->removeCallEdge(CI);
            break;
          }
        }
      }
    }
  }
};

} // namespace notdec::utils

namespace llvm {

using notdec::utils::CallGraphDOTInfo;
using notdec::utils::CallMultiGraph;
using notdec::utils::HideExternalFuncs;
using notdec::utils::ShowEdgeWeight;
using notdec::utils::ShowHeatColors;

template <>
struct GraphTraits<CallGraphDOTInfo *>
    : public GraphTraits<const CallGraphNode *> {
  static NodeRef getEntryNode(CallGraphDOTInfo *CGInfo) {
    // Start at the external node!
    return CGInfo->getCallGraph()->getExternalCallingNode();
  }

  typedef std::pair<const Function *const, std::unique_ptr<CallGraphNode>>
      PairTy;
  static const CallGraphNode *CGGetValuePtr(const PairTy &P) {
    return P.second.get();
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef mapped_iterator<CallGraph::const_iterator, decltype(&CGGetValuePtr)>
      nodes_iterator;

  static nodes_iterator nodes_begin(CallGraphDOTInfo *CGInfo) {
    return nodes_iterator(CGInfo->getCallGraph()->begin(), &CGGetValuePtr);
  }
  static nodes_iterator nodes_end(CallGraphDOTInfo *CGInfo) {
    return nodes_iterator(CGInfo->getCallGraph()->end(), &CGGetValuePtr);
  }
};

template <>
struct DOTGraphTraits<CallGraphDOTInfo *> : public DefaultDOTGraphTraits {

  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}

  static std::string getGraphName(CallGraphDOTInfo *CGInfo) {
    return "Call graph: " +
           std::string(CGInfo->getModule()->getModuleIdentifier());
  }

  static bool isNodeHidden(const CallGraphNode *Node,
                           const CallGraphDOTInfo *CGInfo) {
    if (HideExternalFuncs && Node->getFunction() &&
        Node->getFunction()->isDeclaration()) {
      return true;
    }
    if (CallMultiGraph || Node->getFunction())
      return false;
    return true;
  }

  std::string getNodeLabel(const CallGraphNode *Node,
                           CallGraphDOTInfo *CGInfo) {
    if (Node == CGInfo->getCallGraph()->getExternalCallingNode())
      return "external caller";
    if (Node == CGInfo->getCallGraph()->getCallsExternalNode())
      return "external callee";

    if (Function *Func = Node->getFunction())
      return std::string(Func->getName());
    return "external node";
  }
  static const CallGraphNode *CGGetValuePtr(CallGraphNode::CallRecord P) {
    return P.second;
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef mapped_iterator<CallGraphNode::const_iterator,
                          decltype(&CGGetValuePtr)>
      nodes_iterator;

  std::string getEdgeAttributes(const CallGraphNode *Node, nodes_iterator I,
                                CallGraphDOTInfo *CGInfo) {
    if (!ShowEdgeWeight)
      return "";

    Function *Caller = Node->getFunction();
    if (Caller == nullptr || Caller->isDeclaration())
      return "";

    Function *Callee = (*I)->getFunction();
    if (Callee == nullptr)
      return "";

    uint64_t Counter = llvm::getNumOfCalls(*Caller, *Callee);
    double Width = 1 + 2 * (double(Counter) / CGInfo->getMaxFreq());
    std::string Attrs = "label=\"" + std::to_string(Counter) +
                        "\" penwidth=" + std::to_string(Width);
    return Attrs;
  }

  std::string getNodeAttributes(const CallGraphNode *Node,
                                CallGraphDOTInfo *CGInfo) {
    Function *F = Node->getFunction();
    if (F == nullptr)
      return "";
    std::string attrs;
    if (ShowHeatColors) {
      uint64_t freq = CGInfo->getFreq(F);
      std::string color = getHeatColor(freq, CGInfo->getMaxFreq());
      std::string edgeColor = (freq <= (CGInfo->getMaxFreq() / 2))
                                  ? getHeatColor(0)
                                  : getHeatColor(1);
      attrs = "color=\"" + edgeColor + "ff\", style=filled, fillcolor=\"" +
              color + "80\"";
    }
    return attrs;
  }
};
} // namespace llvm

#endif
