// Phoenix: Native x86 Decompilation Using Semantics-Preserving Structural
// Analysis and Iterative Control-Flow Structuring

#include <cassert>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <iostream>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "backend/CFG.h"
#include "backend/PostOrderCFGView.h"
#include "backend/phoenix.h"
#include "backend/structural-analysis.h"

namespace notdec::backend {

void Phoenix::execute() {
  int iterations = 0;
  do {
    if (isCanceled) {
      break;
    }
    ++iterations;
    if (iterations > 1000) {
      llvm::errs() << "Structure analysis stopped making progress, quitting."
                   << " Func: " << FCtx.getFunction().getName();
      break;
    }

    int oldCount = CFG.size();
    // this will maintain a list, so modification during iteration is OK
    auto postView = PostOrderCFGView::create(&CFG);
    Dom.buildDominatorTree(&CFG);
    unresolvedCycles.clear();
    unresolvedSwitches.clear();
    // Dom.dump();

    for (const CFGBlock *CBlock : *postView) {
      CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
      if (!CFG.contains(Block)) {
        continue;
      }

      bool Changed;
      int round = 0;
      do {
        Changed = false;
        Changed |= ReduceAcyclic(Block);
        if (!Changed && isCyclic(Block)) {
          Changed |= ReduceCyclic(Block);
        }
        round += 1;
      } while (Changed);
    }
    if (CFG.size() == oldCount && CFG.size() > 1) {
      // Didn't make any progress this round,
      // try refining unstructured regions
      ProcessUnresolvedRegions();
    }
  } while (CFG.size() > 1);
}

bool Phoenix::isBackEdge(CFGBlock *A, CFGBlock *B) {
  return Dom.properlyDominates(B, A);
}

bool Phoenix::isCyclic(CFGBlock *Block) {
  for (auto &pred : Block->preds()) {
    // 1 self loop
    if (pred.getBlock() == Block) {
      return true;
    }
    // 2 back edge: edge target dominates source
    if (isBackEdge(pred.getBlock(), Block)) {
      return true;
    }
  }
  return false;
}

void backwardVisit(std::set<CFGBlock *> &visited, CFGBlock *Entry,
                   CFGBlock *Current) {
  assert(visited.insert(Current).second);
  if (Current == Entry) {
    return;
  }
  for (auto &pred : Current->preds()) {
    auto P = pred.getBlock();
    if (visited.find(P) == visited.end()) {
      backwardVisit(visited, Entry, P);
    }
  }
}

/// Visits given node and recursively visits all its GRAY successors.
/// All the visited nodes are painted Black.
void forwardVisit(std::set<CFGBlock *> &visited, std::set<CFGBlock *> gray,
                  CFGBlock *Current) {
  assert(gray.count(Current) == 1);
  assert(visited.insert(Current).second);
  for (auto &pred : Current->succs()) {
    auto P = pred.getBlock();
    if (visited.find(P) == visited.end() && gray.count(P) == 1) {
      forwardVisit(visited, gray, P);
    }
  }
}

std::unique_ptr<std::set<CFGBlock *>> getLoopNodes(CFGBlock *Block,
                                                   CFGDomTree &Dom) {
  std::set<CFGBlock *> Gray;
  std::set<CFGBlock *> Black;
  std::unique_ptr<std::set<CFGBlock *>> ret =
      std::make_unique<std::set<CFGBlock *>>();
  for (auto &pred : Block->preds()) {
    auto P = pred.getBlock();
    if (Dom.properlyDominates(Block, P)) {
      // backward visit from back edge node
      // because Block dom P, back visit from P will find Block eventually.
      if (Gray.count(P) == 0) {
        backwardVisit(Gray, Block, P);
      }
    } else if (P == Block) {
      // self loop
      ret->insert(P);
    }
  }

  if (Gray.find(Block) != Gray.end()) {
    forwardVisit(Black, Gray, Block);
  }
  ret->insert(Black.begin(), Black.end());
  return ret;
}

bool Phoenix::ReduceCyclic(CFGBlock *Block) {
  bool changed = false;
  std::unique_ptr<std::set<CFGBlock *>> loopNodes = getLoopNodes(Block, Dom);
  while (true) {
    if (Block->succ_size() != 1 || !reduceSequence(Block)) {
      break;
    }
    changed = true;
  }
  // handle any self loop edges, and convert to while loop
  for (auto &succ : Block->succs()) {
    auto S = succ.getBlock();
    if (S == Block) {
      clang::Stmt *whil;
      if (Block->succ_size() == 1) {
        // infinite loop
        // create while(true){stmts}
        auto body = makeCompoundStmt(Block, true);
        whil = createWhileTrue(body);
      } else {
        assert(Block->succ_size() == 2);
        // Do while loop
        auto body = makeCompoundStmt(Block, true);
        auto cond = takeBinaryCond(*Block);
        // invert if loop is the false branch
        if (Block->isFalseBrSucc(S)) {
          cond = invertCond(cond);
        }
        // create do while stmt
        whil = new (Ctx)
            clang::DoStmt(body, cond, clang::SourceLocation(),
                          clang::SourceLocation(), clang::SourceLocation());
      }
      // organize edges: now it has only one successor.
      Block->appendStmt(whil);
      removeEdge(Block, Block);
      return true;
    }
  }
  // Should be condition. Switches should not match a cyclic pattern
  if (Block->succ_size() > 2) {
    return changed;
  }
  for (auto &succ : Block->succs()) {
    auto S = succ.getBlock();
    // While loop: Block -> S -> Block, and S has no other succ or pred
    if (linearSuccessor(S) == Block && singlePredecessor(S) == Block) {
      auto cond = takeBinaryCond(*Block);
      // invert cond if the enter the
      if (Block->isFalseBrSucc(S)) {
        cond = invertCond(cond);
      }
      clang::Stmt *whil;
      if (Block->size() == 0) {
        // S is the only body block
        auto body = makeCompoundStmt(S);
        whil = clang::WhileStmt::Create(
            Ctx, nullptr, cond, body, clang::SourceLocation(),
            clang::SourceLocation(), clang::SourceLocation());
      } else {
        // Wrap in a big while true, and use break to leave
        // while (true) {Block; if(cond) break; S;}
        std::vector<clang::Stmt *> stmts;
        // add all stmts in Block and clear.
        for (auto elem = Block->begin(); elem != Block->end(); ++elem) {
          if (auto stmt = elem->getAs<CFGStmt>()) {
            stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
          }
        }
        Block->clear();
        // add a if-break stmt.
        auto if_break = clang::IfStmt::Create(
            Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
            nullptr, nullptr, cond, clang::SourceLocation(),
            clang::SourceLocation(),
            new (Ctx) clang::BreakStmt(clang::SourceLocation()));
        stmts.push_back(if_break);
        // add stmts in the S block
        for (auto elem = S->begin(); elem != S->end(); ++elem) {
          if (auto stmt = elem->getAs<CFGStmt>()) {
            stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
          }
        }
        // create the while true
        auto body = clang::CompoundStmt::Create(
            Ctx, stmts, clang::SourceLocation(), clang::SourceLocation());
        whil = createWhileTrue(body);
      }
      // now the while is the only stmt in Block
      Block->appendStmt(whil);
      removeEdge(Block, S);
      removeEdge(S, Block);
      CFG.remove(S);
      return true;
    } // end Block -> S -> Block
  }
  // It's a cyclic region, but we are unable to collapse it.
  // Schedule it for refinement after the whole graph has been
  // traversed.
  // enqueueUnresolvedLoop(Block, loopNodes);
  // Do not refine cycle if there are unresolved switches
  if (unresolvedSwitches.size() == 0) {
    unresolvedCycles.emplace_back(Block, std::move(*loopNodes));
  }
  return changed;
}

// count the number of preds of block, that is not in the loopNodes.
int countIncomingEdges(CFGBlock *block, std::set<CFGBlock *> &loopNodes) {
  int count = 0;
  for (auto &pred : block->preds()) {
    // not in the set
    if (loopNodes.count(pred.getBlock()) == 0) {
      count += 1;
    }
  }
  return count;
}

/// Ensure the loop has a single entrance (nodes with
/// incoming edges from outside the loop).  If there are
/// multiple entrances to the loop, we select the one with
/// the most incoming edges, and virtualize the other
/// entrance edges.
/// Currently the virtualization part is not implemented.
/// TODO any negative impact?
CFGBlock *ensureSingleEntry(CFGBlock *head, std::set<CFGBlock *> &loopNodes) {
  // first ensure there is only one entry
  auto cinMax = countIncomingEdges(head, loopNodes);
  CFGBlock *headMax = head;
  for (auto n : loopNodes) {
    auto cin = countIncomingEdges(n, loopNodes);
    if (cin > cinMax) {
      cinMax = cin;
      headMax = n;
    }
  }
  return headMax;
}

/// get blocks reachable from n, not via head.
/// blocks contains the visited result.
void findReachableBlocks(CFGBlock *n, CFGBlock *head,
                         std::set<CFGBlock *> blocks) {
  blocks.insert(n);
  for (auto &succ : n->succs()) {
    if (blocks.count(succ.getBlock()) == 0 && succ.getBlock() != head) {
      findReachableBlocks(succ.getBlock(), head, blocks);
    }
  }
}

/// in Section 3.6, the updated version of lexical nodes.
/// original lexical nodes(i.e. body of the loop):
/// - 一个反向边的Natural Loop（即，natural loop严格说并不是一个单独的概念，
/// 反而是对一条反向边而言的。），是指，最小的，包括反向边头和尾节点的节点集合，
/// 集合内所有节点的前驱，要么在集合内，要么这个节点是entry节点，前驱是entry的前驱。
///
/// new additional lexical nodes: (additional nodes besides the above nodes.)
/// - dominated by the loop header, excluding any nodes reachable from the
/// loop’s successor without going through the loop header
std::set<CFGBlock *> Phoenix::getLexicalNodes(CFGBlock *head, CFGBlock *follow,
                                              std::set<CFGBlock *> &loopNodes) {
  std::set<CFGBlock *> excluded;
  // find reachable nodes, starting from follow, and without passing through
  // head node. these nodes are excluded.
  findReachableBlocks(follow, head, excluded);
  std::set<CFGBlock *> lexNodes;
  // starting from loopNodes.
  std::queue<CFGBlock *> queue;
  for (auto node : loopNodes) {
    queue.push(node);
  }
  while (!queue.empty()) {
    CFGBlock *fr = queue.front();
    queue.pop();
    if (loopNodes.count(fr)) {
      // if in loopNodes, insert and push succs to queue.
      lexNodes.insert(fr);
      for (auto &succ : fr->succs()) {
        if (lexNodes.count(succ.getBlock()) == 0) {
          queue.push(succ.getBlock());
        }
      }
    } else if (Dom.properlyDominates(head, fr) && (excluded.count(fr) == 0)) {
      // if dominated by head, and not in exclude list
      lexNodes.insert(fr);
      for (auto &succ : fr->succs()) {
        if (lexNodes.count(succ.getBlock()) == 0) {
          queue.push(succ.getBlock());
        }
      }
    }
  }
  return lexNodes;
}

std::pair<CFGBlock *, CFGBlock *>
Phoenix::determineFollowLatch(CFGBlock *head, std::set<CFGBlock *> &loopNodes) {
  if (head->succ_size() == 2) {
    auto succs = head->getTwoSuccs();
    // If the head is a Conditional node and one of the edges
    // leaves the loop, the head of that edge is the follow
    // node of the loop.
    CFGBlock *follow = nullptr;
    if (loopNodes.count(succs.first) == 0) {
      follow = succs.first;
    } else if (loopNodes.count(succs.second) == 0) {
      follow = succs.second;
    }
    if (follow != nullptr) {
      for (auto &latch : head->succs()) {
        if (isBackEdge(latch, head) && linearSuccessor(latch) == head) {
          return std::make_pair(follow, latch);
        }
      }
    }
  }
  // find back edges: latch -> head, and check other succ of latch that is not
  // in the loop.
  for (auto &latch : head->preds()) {
    if (isBackEdge(latch.getBlock(), head)) {
      if (latch->succ_size() == 2) {
        auto succs = latch->getTwoSuccs();
        if (loopNodes.count(succs.first) == 0) {
          return std::make_pair(succs.first, latch);
        }
        if (loopNodes.count(succs.second) == 0) {
          return std::make_pair(succs.second, latch);
        }
      }
    }
  }
  return std::make_pair(nullptr, nullptr);
}

// has any edge from N -> follow
bool hasExitEdgeFrom(CFGBlock *N, CFGBlock *Follow) {
  for (auto &succ : N->succs()) {
    if (succ.getBlock() == Follow) {
      return true;
    }
  }
  return false;
}

Phoenix::LoopType determineLoopType(CFGBlock *head, CFGBlock *latch,
                                    CFGBlock *follow) {
  if (!hasExitEdgeFrom(latch, follow)) {
    return Phoenix::While;
  }
  if (!hasExitEdgeFrom(head, follow)) {
    return Phoenix::DoWhile;
  }
  if (head->size() > 0) {
    return Phoenix::DoWhile;
  }
  return Phoenix::While;
}

bool isPureReturn(CFGBlock *B) {
  if (B->size() != 1) {
    return false;
  }
  auto stmt = B->front().castAs<CFGStmt>().getStmt();
  return llvm::isa<clang::ReturnStmt>(stmt);
}

void Phoenix::collapseToTailRegion(CFGBlock *From, CFGBlock *To,
                                   clang::Stmt *stm) {
  switch (From->succ_size()) {
  case 2: {
    auto cond = takeBinaryCond(*From);
    if (From->isFalseBrSucc(To)) {
      cond = invertCond(cond);
    }
    auto IfStmt = clang::IfStmt::Create(
        Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary, nullptr,
        nullptr, cond, clang::SourceLocation(), clang::SourceLocation(), stm);
    From->appendStmt(IfStmt);
  } break;
  case 1:
    From->appendStmt(stm);
    break;
  default:
    CFG.dump(Ctx.getLangOpts(), true);
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Error: Can't collapse edge! function: "
                 << FCtx.getFunction().getName()
                 << ", edge: " << From->getBlockID() << " -> "
                 << To->getBlockID() << "\n";
    std::abort();
  }
  removeEdge(From, To);
}

void Phoenix::virtualizeEdge(Phoenix::VirtualEdge &edge) {
  clang::Stmt *stm;
  if (isPureReturn(edge.To)) {
    // Goto to a return statement => just a return statement.
    auto ret =
        (clang::ReturnStmt *)edge.To->front().castAs<CFGStmt>().getStmt();
    stm = clang::ReturnStmt::Create(Ctx, clang::SourceLocation(),
                                    ret->getRetValue(), nullptr);
  } else {
    switch (edge.Ty) {
    case Break:
      stm = new (Ctx) clang::BreakStmt(clang::SourceLocation());
      break;
    case Continue:
      stm = new (Ctx) clang::ContinueStmt(clang::SourceLocation());
      break;
    case Goto: {
      auto label = getBlockLabel(edge.To, true);
      stm = new (Ctx) clang::GotoStmt(label, clang::SourceLocation(),
                                      clang::SourceLocation());
      break;
    }
    default:
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Error: unhandled enum of Phoenix::VirtialEdge" << std::endl;

      std::abort();
    }
  }
  collapseToTailRegion(edge.From, edge.To, stm);
  if (edge.To->pred_size() == 0 && edge.To != &CFG.getEntry()) {
    if (isPureReturn(edge.To)) {
      CFG.remove(edge.To);
    } else {
      CFG.dump(Ctx.getLangOpts(), true);
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Error: Removing edge (" << edge.From->getBlockID() << ", "
                << edge.To->getBlockID() << ") caused loss of some code blocks"
                << std::endl;
      std::abort();
    }
  }
}

bool Phoenix::virtualizeIrregularExits(CFGBlock *head, CFGBlock *latch,
                                       CFGBlock *follow,
                                       std::set<CFGBlock *> &lexicalNodes) {
  bool changed = false;
  Phoenix::LoopType loopType = determineLoopType(head, latch, follow);
  std::vector<Phoenix::VirtualEdge> vEdges;
  for (auto n : lexicalNodes) {
    for (auto &s : n->succs()) {
      if (s == head) {
        if (n != latch) {
          vEdges.emplace_back(n, s, Phoenix::VirtualEdgeType::Continue);
        }
      } else if (lexicalNodes.count(s) == 0) {
        if (s == follow) {
          if ((loopType == Phoenix::DoWhile && n != latch) ||
              (loopType == Phoenix::While && n != head)) {
            vEdges.emplace_back(n, s, Phoenix::VirtualEdgeType::Break);
          }
        }
      } else {
        vEdges.emplace_back(n, s, Phoenix::VirtualEdgeType::Goto);
      }
    }
    for (auto &edge : vEdges) {
      changed = true;
      virtualizeEdge(edge);
    }
  }
  return changed;
}

bool Phoenix::coalesceTailRegion(CFGBlock *n, std::set<CFGBlock *> &range) {
  if (n->succ_size() == 2) {
    auto ss = n->getTwoSuccs();
    auto th = ss.first;
    auto el = ss.second;

    if (el->succ_size() == 0 && th->succ_size() == 0 &&
        singlePredecessor(el) == n && singlePredecessor(th) == n) {
      auto cond = takeBinaryCond(*n);
      auto then = makeCompoundStmt(th);
      auto els = makeCompoundStmt(el);
      auto IfStmt = clang::IfStmt::Create(
          Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
          nullptr, nullptr, cond, clang::SourceLocation(),
          clang::SourceLocation(), then, clang::SourceLocation(), els);
      n->appendStmt(IfStmt);
      removeEdge(n, th);
      removeEdge(n, el);
      CFG.remove(th);
      CFG.remove(el);
      return true;
    }
    if (range.count(el) != 0 && el->succ_size() == 0 &&
        singlePredecessor(el) == n) {
      auto cond = takeBinaryCond(*n);
      cond = invertCond(cond);
      auto body = makeCompoundStmt(el);
      auto ifStmt = clang::IfStmt::Create(
          Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
          nullptr, nullptr, cond, clang::SourceLocation(),
          clang::SourceLocation(), body, clang::SourceLocation());
      n->appendStmt(ifStmt);
      removeEdge(n, el);
      CFG.remove(el);
      return true;
    };
    if (range.count(th) != 0 && th->succ_size() == 0 &&
        singlePredecessor(th) == n) {
      auto cond = takeBinaryCond(*n);
      auto body = makeCompoundStmt(th);
      auto ifStmt = clang::IfStmt::Create(
          Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
          nullptr, nullptr, cond, clang::SourceLocation(),
          clang::SourceLocation(), body, clang::SourceLocation());
      n->appendStmt(ifStmt);
      removeEdge(n, th);
      CFG.remove(th);
      return true;
    }
    // TODO
  }
  return false;
}

bool Phoenix::refineLoop(CFGBlock *head, std::set<CFGBlock *> &loopNodes) {
  // find the single entry.
  head = ensureSingleEntry(head, loopNodes);
  // back edge: latch -> head. follow node is the node after the loop.
  auto follow_latch = determineFollowLatch(head, loopNodes);
  auto follow = follow_latch.first;
  auto latch = follow_latch.second;
  if (follow == nullptr && latch == nullptr) {
    return false;
  }
  auto lexicalNodes = getLexicalNodes(head, follow, loopNodes);
  auto virtualized =
      virtualizeIrregularExits(head, latch, follow, lexicalNodes);
  if (virtualized) {
    return true;
  }
  for (auto n : lexicalNodes) {
    if (coalesceTailRegion(n, lexicalNodes)) {
      return true;
    }
  }
  return lastResort(lexicalNodes);
}

llvm::Optional<Phoenix::VirtualEdge>
Phoenix::findLastResortEdge(std::set<CFGBlock *> &blocks) {
  std::vector<Phoenix::VirtualEdge> edges;
  // get all in region edges.
  for (auto n : blocks) {
    for (auto &s : n->succs()) {
      if (blocks.count(s) != 0) {
        edges.emplace_back(n, s.getBlock(), Phoenix::VirtualEdgeType::Goto);
      }
    }
  }
  for (auto &vEdge : edges) {
    if (!Dom.properlyDominates(vEdge.From, vEdge.To) &&
        !Dom.properlyDominates(vEdge.To, vEdge.From)) {
      return vEdge;
    }
  }
  for (auto &vEdge : edges) {
    if (!Dom.properlyDominates(vEdge.From, vEdge.To)) {
      return vEdge;
    }
  }
  if (edges.empty()) {
    return llvm::None;
  } else {
    return edges.front();
  }
}

bool Phoenix::lastResort(std::set<CFGBlock *> &blocks) {
  auto vEdge = findLastResortEdge(blocks);
  if (vEdge.hasValue()) {
    virtualizeEdge(*vEdge);
    return true;
  } else {
    // Whoa, we're in trouble now....
    return false;
  }
}

void Phoenix::refineIncSwitch(CFGBlock *switchHead) {
  std::cerr << "unimplemented";
  std::abort();
}

bool Phoenix::virtualizeReturn(CFGBlock *n) {
  llvm::Optional<Phoenix::VirtualEdge> returnEdge;
  for (auto &s : n->succs()) {
    if (isPureReturn(s)) {
      returnEdge.emplace(n, s, VirtualEdgeType::Goto);
      break;
    }
  }
  if (returnEdge.hasValue()) {
    virtualizeEdge(*returnEdge);
    return true;
  }
  return false;
}

bool Phoenix::ProcessUnresolvedRegions() {
  if (unresolvedCycles.size() > 0) {
    auto cycle = *unresolvedCycles.erase(unresolvedCycles.begin());
    if (refineLoop(cycle.first, cycle.second)) {
      return true;
    }
  }
  if (unresolvedSwitches.size() > 0) {
    auto switchHead = *unresolvedSwitches.erase(unresolvedSwitches.begin());
    refineIncSwitch(switchHead);
    return true;
  }
  auto postView = PostOrderCFGView::create(&CFG);
  for (const CFGBlock *CBlock : *postView) {
    CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
    if (virtualizeReturn(Block)) {
      return true;
    }
  }
  std::set<CFGBlock *> nodes(CFG.nodes_begin(), CFG.nodes_end());
  for (const CFGBlock *CBlock : *postView) {
    CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
    if (coalesceTailRegion(Block, nodes)) {
      return true;
    }
  }
  for (const CFGBlock *CBlock : *postView) {
    CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
    if (lastResort(Block)) {
      return true;
    }
  }
  return false;
}

bool Phoenix::lastResort(CFGBlock *n) {
  llvm::Optional<Phoenix::VirtualEdge> vEdge;
  for (auto &s : n->succs()) {
    if (!Dom.properlyDominates(n, s) && !Dom.properlyDominates(s, n)) {
      vEdge.emplace(n, s, VirtualEdgeType::Goto);
      break;
    }
  }
  if (!vEdge.hasValue()) {
    for (auto &s : n->succs()) {
      if (!Dom.properlyDominates(n, s)) {
        vEdge.emplace(n, s, VirtualEdgeType::Goto);
        break;
      }
    }
  }
  if (!vEdge.hasValue()) {
    for (auto &p : n->preds()) {
      if (!Dom.properlyDominates(p, n)) {
        vEdge.emplace(p, n, VirtualEdgeType::Goto);
      }
    }
  }
  if (vEdge.hasValue()) {
    virtualizeEdge(*vEdge);
    return true;
  } else {
    // Whoa, we're in trouble now....
    return false;
  }
}

bool Phoenix::reduceSequence(CFGBlock *Block) {
  assert(Block->succ_size() == 1);
  CFGBlock *Succ = *Block->succ_begin();
  if (Succ->pred_size() == 1) {
    // merge two blocks
    // 1 add all instructions in succ to current block
    for (auto elem = Succ->begin(); elem != Succ->end(); ++elem) {
      Block->appendElement(*elem);
    }
    Block->setTerminator(Succ->getTerminator());
    // remove this edge
    removeEdge(Block, Succ);
    // add all edges from succ to current block
    replaceSuccessors(Succ, Block);
    // TODO faster remove instead of linear search.
    CFG.remove(Succ);
    return true;
  } else {
    return false;
  }
}

bool Phoenix::reduceIfRegion(CFGBlock *Block) {
  auto tmp = Block->getTwoSuccs();
  CFGBlock *th = tmp.first;
  CFGBlock *el = tmp.second;
  assert(th != nullptr && el != nullptr);
  CFGBlock *elS = linearSuccessor(el);
  CFGBlock *thS = linearSuccessor(th);

  // the successor of else is then block.
  if (elS == th && onlyPred(el, Block)) {
    clang::Expr *cond = takeBinaryCond(*Block);
    // collapse the else block: if(!cond){else} elseSucc/then
    cond = invertCond(cond);
    auto then = makeCompoundStmt(el);
    auto IfStmt = clang::IfStmt::Create(
        Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary, nullptr,
        nullptr, cond, clang::SourceLocation(), clang::SourceLocation(), then,
        clang::SourceLocation());
    Block->appendStmt(IfStmt);
    // maintain the edge
    removeEdge(Block, el);
    removeEdge(el, elS);
    CFG.remove(el);
    assert(Block->succ_size() == 1);
    return true;
  } else if (thS == el && onlyPred(th, Block)) {
    clang::Expr *cond = takeBinaryCond(*Block);
    // collapse the then block: if(cond){then} thenSucc/else;
    auto then = makeCompoundStmt(th);
    auto IfStmt = clang::IfStmt::Create(
        Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary, nullptr,
        nullptr, cond, clang::SourceLocation(), clang::SourceLocation(), then,
        clang::SourceLocation());
    Block->appendStmt(IfStmt);
    // maintain the edge
    removeEdge(Block, th);
    removeEdge(th, thS);
    CFG.remove(th);
    assert(Block->succ_size() == 1);
    return true;
  } else if (elS != nullptr && elS == thS) {
    if (!(onlyPred(el, Block) && onlyPred(th, Block))) {
      return false;
    }
    clang::Expr *cond = takeBinaryCond(*Block);
    // collapse the then and else block: if(cond){thenBlk}else{elseBlk}
    // thenSucc/elseSucc;
    auto then = makeCompoundStmt(th);
    auto els = makeCompoundStmt(el);
    auto IfStmt = clang::IfStmt::Create(
        Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary, nullptr,
        nullptr, cond, clang::SourceLocation(), clang::SourceLocation(), then,
        clang::SourceLocation(), els);
    Block->appendStmt(IfStmt);
    // maintain the edge
    removeEdge(Block, th);
    removeEdge(Block, el);
    removeEdge(th, thS);
    removeEdge(el, elS);
    CFG.remove(th);
    CFG.remove(el);
    Block->addSuccessor(elS);
    assert(Block->succ_size() == 1);
    return true;
  }
  return false;
}

bool Phoenix::ReduceAcyclic(CFGBlock *Block) {
  switch (Block->succ_size()) {
  case 0:
    return false;
    break;
  case 1:
    // reduce sequence
    return reduceSequence(Block);
    break;
  case 2:
    // reduce if-else
    return reduceIfRegion(Block);
    break;
  default:
    // TODO support switch
    assert(false && "Not implemented");
    break;
  }
  // unreachable
  assert(false && "unreachable");
}

} // namespace notdec::backend
