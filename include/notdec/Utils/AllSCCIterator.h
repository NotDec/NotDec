#ifndef _NOTDEC_UTILS_ALLSCC_H_
#define _NOTDEC_UTILS_ALLSCC_H_

#include <llvm/ADT/SCCIterator.h>
#include <llvm/ADT/iterator_range.h>

namespace notdec {

using llvm::DenseMap;
using llvm::GraphTraits;
using llvm::iterator_facade_base;

/// Enumerate the SCCs of a directed graph in reverse topological order
/// of the SCC DAG.
///
/// This is implemented using Tarjan's DFS algorithm using an internal stack to
/// build up a vector of nodes in a particular SCC. Note that it is a forward
/// iterator and thus you cannot backtrack or re-visit nodes.
template <class GraphT, class GT = GraphTraits<GraphT>>
class all_scc_iterator
    : public iterator_facade_base<
          all_scc_iterator<GraphT, GT>, std::forward_iterator_tag,
          const std::vector<typename GT::NodeRef>, ptrdiff_t> {
protected:
  using NodeRef = typename GT::NodeRef;
  using ChildItTy = typename GT::ChildIteratorType;
  using SccTy = std::vector<NodeRef>;
  using reference = typename all_scc_iterator::reference;

  /// Element of VisitStack during DFS.
  struct StackElement {
    NodeRef Node;        ///< The current node pointer.
    ChildItTy NextChild; ///< The next child, modified inplace during DFS.
    unsigned MinVisited; ///< Minimum uplink value of all children of Node.

    StackElement(NodeRef Node, const ChildItTy &Child, unsigned Min)
        : Node(Node), NextChild(Child), MinVisited(Min) {}

    bool operator==(const StackElement &Other) const {
      return Node == Other.Node && NextChild == Other.NextChild &&
             MinVisited == Other.MinVisited;
    }
  };

  const GraphT G;
  /// The visit counters used to detect when a complete SCC is on the stack.
  /// visitNum is the global counter.
  ///
  /// nodeVisitNumbers are per-node visit numbers, also used as DFS flags.
  unsigned visitNum;
  DenseMap<NodeRef, unsigned> nodeVisitNumbers;

  /// Stack holding nodes of the SCC.
  std::vector<NodeRef> SCCNodeStack;

  /// The current SCC, retrieved using operator*().
  SccTy CurrentSCC;

  /// DFS stack, Used to maintain the ordering.  The top contains the current
  /// node, the next child to visit, and the minimum uplink value of all child
  std::vector<StackElement> VisitStack;

  /// A single "visit" within the non-recursive DFS traversal.
  void DFSVisitOne(NodeRef N);

  /// The stack-based DFS traversal; defined below.
  void DFSVisitChildren();

  /// Compute the next SCC using the DFS traversal.
  void GetNextSCC();

  all_scc_iterator(const GraphT G) : G(G), visitNum(0) {
    DFSVisitOne(GT::getEntryNode(G));
    GetNextSCC();
  }

  /// End is when the DFS stack is empty.
  all_scc_iterator() = default;

public:
  static all_scc_iterator begin(const GraphT &G) { return all_scc_iterator(G); }
  static all_scc_iterator end(const GraphT &) { return all_scc_iterator(); }

  /// Direct loop termination test which is more efficient than
  /// comparison with \c end().
  bool isAtEnd() const {
    assert(!CurrentSCC.empty() || VisitStack.empty());
    return CurrentSCC.empty();
  }

  bool operator==(const all_scc_iterator &x) const {
    return VisitStack == x.VisitStack && CurrentSCC == x.CurrentSCC;
  }

  all_scc_iterator &operator++() {
    GetNextSCC();
    return *this;
  }

  reference operator*() const {
    assert(!CurrentSCC.empty() && "Dereferencing END SCC iterator!");
    return CurrentSCC;
  }

  /// Test if the current SCC has a cycle.
  ///
  /// If the SCC has more than one node, this is trivially true.  If not, it may
  /// still contain a cycle if the node has an edge back to itself.
  bool hasCycle() const;

  /// This informs the \c all_scc_iterator that the specified \c Old node
  /// has been deleted, and \c New is to be used in its place.
  void ReplaceNode(NodeRef Old, NodeRef New) {
    assert(nodeVisitNumbers.count(Old) && "Old not in all_scc_iterator?");
    // Do the assignment in two steps, in case 'New' is not yet in the map, and
    // inserting it causes the map to grow.
    auto tempVal = nodeVisitNumbers[Old];
    nodeVisitNumbers[New] = tempVal;
    nodeVisitNumbers.erase(Old);
  }
};

template <class GraphT, class GT>
void all_scc_iterator<GraphT, GT>::DFSVisitOne(NodeRef N) {
  ++visitNum;
  nodeVisitNumbers[N] = visitNum;
  SCCNodeStack.push_back(N);
  VisitStack.push_back(StackElement(N, GT::child_begin(N), visitNum));
#if 0 // Enable if needed when debugging.
  dbgs() << "TarjanSCC: Node " << N <<
        " : visitNum = " << visitNum << "\n";
#endif
}

template <class GraphT, class GT>
void all_scc_iterator<GraphT, GT>::DFSVisitChildren() {
  assert(!VisitStack.empty());
  while (VisitStack.back().NextChild != GT::child_end(VisitStack.back().Node)) {
    // TOS has at least one more child so continue DFS
    NodeRef childN = *VisitStack.back().NextChild++;
    typename DenseMap<NodeRef, unsigned>::iterator Visited =
        nodeVisitNumbers.find(childN);
    if (Visited == nodeVisitNumbers.end()) {
      // this node has never been seen.
      DFSVisitOne(childN);
      continue;
    }

    unsigned childNum = Visited->second;
    if (VisitStack.back().MinVisited > childNum)
      VisitStack.back().MinVisited = childNum;
  }
}

template <class GraphT, class GT>
void all_scc_iterator<GraphT, GT>::GetNextSCC() {
  CurrentSCC.clear(); // Prepare to compute the next SCC
  if (VisitStack.empty()) {
    // find a node that has not been visited yet
    for (auto N : llvm::make_range(GT::nodes_begin(const_cast<GraphT>(G)),
                                   GT::nodes_end(const_cast<GraphT>(G)))) {
      if (nodeVisitNumbers.find(N) == nodeVisitNumbers.end()) {
        DFSVisitOne(N);
        break;
      }
    }
  }
  while (!VisitStack.empty()) {
    DFSVisitChildren();

    // Pop the leaf on top of the VisitStack.
    NodeRef visitingN = VisitStack.back().Node;
    unsigned minVisitNum = VisitStack.back().MinVisited;
    assert(VisitStack.back().NextChild == GT::child_end(visitingN));
    VisitStack.pop_back();

    // Propagate MinVisitNum to parent so we can detect the SCC starting node.
    if (!VisitStack.empty() && VisitStack.back().MinVisited > minVisitNum)
      VisitStack.back().MinVisited = minVisitNum;

#if 0 // Enable if needed when debugging.
    dbgs() << "TarjanSCC: Popped node " << visitingN <<
          " : minVisitNum = " << minVisitNum << "; Node visit num = " <<
          nodeVisitNumbers[visitingN] << "\n";
#endif

    if (minVisitNum != nodeVisitNumbers[visitingN])
      continue;

    // A full SCC is on the SCCNodeStack!  It includes all nodes below
    // visitingN on the stack.  Copy those nodes to CurrentSCC,
    // reset their minVisit values, and return (this suspends
    // the DFS traversal till the next ++).
    do {
      CurrentSCC.push_back(SCCNodeStack.back());
      SCCNodeStack.pop_back();
      nodeVisitNumbers[CurrentSCC.back()] = ~0U;
    } while (CurrentSCC.back() != visitingN);
    return;
  }
}

template <class GraphT, class GT>
bool all_scc_iterator<GraphT, GT>::hasCycle() const {
  assert(!CurrentSCC.empty() && "Dereferencing END SCC iterator!");
  if (CurrentSCC.size() > 1)
    return true;
  NodeRef N = CurrentSCC.front();
  for (ChildItTy CI = GT::child_begin(N), CE = GT::child_end(N); CI != CE; ++CI)
    if (*CI == N)
      return true;
  return false;
}

/// Construct the begin iterator for a deduced graph type T.
template <class T> all_scc_iterator<T> scc_begin(const T &G) {
  return all_scc_iterator<T>::begin(G);
}

/// Construct the end iterator for a deduced graph type T.
template <class T> all_scc_iterator<T> scc_end(const T &G) {
  return all_scc_iterator<T>::end(G);
}

} // namespace notdec

#endif
