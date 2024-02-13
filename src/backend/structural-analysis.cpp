

#include "backend/structural-analysis.h"
#include "backend/phoenix.h"
#include "backend/region-graph.h"
#include <utility>

namespace notdec::backend {

std::string printBasicBlock(const llvm::BasicBlock *b) {
  return llvmObjToString(b);
}

/// Decompile the module to c and print to a file.
void decompileModule(llvm::Module &M, llvm::raw_fd_ostream &os) {
  SAContext ctx(const_cast<llvm::Module &>(M));
  for (const llvm::Function &F : M) {
    SAFuncContext &funcCtx =
        ctx.getFuncContext(const_cast<llvm::Function &>(F));
    funcCtx.init();
    // TODO: create structural analysis according to cmdline
    Phoenix sa(funcCtx);
    sa.execute();

    // print the function decl
    // print the region
  }
}

std::string printFunction(const llvm::Function *F) {
  std::string str;
  llvm::raw_string_ostream Out(str);
  if (F->isDeclaration()) {
    Out << "declare ";
    Out << ' ';
  } else {
    Out << "define ";
  }

  Out << F->getName();
  Out << " {";
  // Output all of the function's basic blocks.
  for (const llvm::BasicBlock &BB : *F)
    Out << printBasicBlock(&BB);

  Out << "}\n\n";
  return Out.str();
}

void SAFuncContext::init() {
  // 1. build the regions
  LLVMRegionBuilder builder(*this);
  assert(rg.regions.empty() && "regions should be empty, initialized twice?");
  for (llvm::BasicBlock &bb : func) {
    // new regions belong to this vector
    rg.regions.push_back(builder.build(bb)); // ! build the region
    assert(rg.addNode(rg.regions.back()));   // assert insert success
    btor[&bb] = &rg.regions.back();
  }
  // 2. connect edges in the region graph
  for (auto *reg : rg.Nodes) {
    auto bb = reg->getHead();
    for (llvm::BasicBlock *succ : successors(bb)) {
      Region *s = btor.at(succ);
      // new edges belong to this vector
      reg->edges.emplace_back(*s);
      rg.connect(*reg, *s, reg->edges.back());
    }
  }
  // detect or remove non entry and unreachable region?
}

Region LLVMRegionBuilder::build(llvm::BasicBlock &bb) {
  this->bb = &bb;
  Region r(successor2Rty(bb.getTerminator()->getNumSuccessors()), &bb);
  this->region = &r;

  visit(bb);
  // In order to be faithful to the inst order of the LLVM IR,
  // TODO! scan for, in this basic block, not-referenced expression, and create
  // a variable for it. keep the order of the statements. llvm::BitVector
  // referenced(bb.size(), false);

  return r;
}

} // namespace notdec::backend
