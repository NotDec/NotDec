#ifndef _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_
#define _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_

#include <cassert>
#include <deque>
#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>

namespace notdec {

using namespace llvm;

struct RetypdRunner : PassInfoMixin<RetypdRunner> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &);
};

struct TypeVariable {
  using offset_t = std::variant<long, std::string>;
  std::string name;
  std::string labels;
  offset_t currentOffset = 0;
  bool isPrimitive = false;

  std::string str() const {
    assert(std::get<long>(currentOffset) == 0 &&
           "TypeVariable::str: currentOffset not zero");
    return name + labels;
  }
  std::variant<long, std::string> takeOffset() {
    auto ret = currentOffset;
    currentOffset = 0;
    return ret;
  }
  std::string takeOffsetStr() {
    if (std::holds_alternative<long>(currentOffset)) {
      auto ret = std::get<long>(currentOffset);
      currentOffset = 0;
      return std::to_string(ret);
    } else if (std::holds_alternative<std::string>(currentOffset)) {
      auto ret = std::get<std::string>(currentOffset);
      currentOffset = 0;
      return ret;
    }
    assert(false && "TypeVariable::takeOffsetStr: unknown variant!");
  }
  void addOffset(offset_t &other) {
    if (std::holds_alternative<long>(currentOffset) &&
        std::holds_alternative<long>(other)) {
      currentOffset = std::get<long>(currentOffset) + std::get<long>(other);
    } else if (std::holds_alternative<std::string>(currentOffset) &&
               std::holds_alternative<long>(other)) {
      // keep the string
    } else if (std::holds_alternative<long>(currentOffset) &&
               std::holds_alternative<std::string>(other)) {
      currentOffset = std::get<std::string>(other);
    } else if (std::holds_alternative<std::string>(currentOffset) &&
               std::holds_alternative<std::string>(other)) {
      if (std::get<std::string>(currentOffset) !=
          std::get<std::string>(other)) {
        // create warning
        std::cerr << "TypeVariable::addOffset: different string offsets: "
                  << std::get<std::string>(currentOffset) << " and "
                  << std::get<std::string>(other) << "\n";
      }
      currentOffset = std::get<std::string>(currentOffset);
    } else {
      assert(false && "TypeVariable::addOffset: unknown variant!");
    }
  }
};

struct RetypdGenerator {
  std::map<Value *, TypeVariable> Val2Dtv;
  std::map<Function *, std::vector<std::string>> func_constrains;
  std::map<Function *, std::set<Function *>> call_graphs;
  std::string data_layout;
  std::vector<std::string> *current;
  llvm::Value *StackPointer;
  unsigned pointer_size = 0;

  void run(Module &M, llvm::Value *StackPointer);

protected:
  std::map<std::string, long> callInstanceId;
  void run(Function &M);

public:
  static const char *Stack;
  static const char *Memory;

  void gen_call_graph(Module &M);
  void gen_json(std::string outputFilename);

  TypeVariable &setTypeVar(Value *val, const TypeVariable &dtv) {
    auto ref = Val2Dtv.emplace(val, dtv);
    assert(ref.second && "setTypeVar: Value already exists");
    return ref.first->second;
  }
  void addConstraint(const TypeVariable &sub, const TypeVariable &sup) {
    if (sub.isPrimitive && sup.isPrimitive) {
      assert(sub.name == sup.name &&
             "addConstraint: different primitive types !?");
      return;
    }
    current->push_back(sub.str() + " <= " + sup.str());
  }
  TypeVariable &getTypeVar(Value *val);
  TypeVariable getTypeVarNoCache(Value *Val);
  // get a new type_var name from type_val_id
  std::string getNewName(Value &Val, const char *prefix = "v_");
  std::string getFuncName(Function &F);

  TypeVariable deref(Value *val, long size, const char *mode);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static std::string make_deref(const char *Mode, long BitSize,
                                const char *OffsetStr);
  static std::string offset(APInt Offset, int Count = 0);
  static std::string sanitize_name(std::string s);

protected:
  size_t typeValId = 0;

  struct PcodeOpType {
    const char *output;
    // allow for uniform initializations
    const char **inputs;
    int size;

  public:
    PcodeOpType(const char *output, const char *inputs[], int size)
        : output(output), inputs(inputs), size(size) {}
    void addConstrains(Instruction *I, RetypdGenerator &cg) const;
  };

  static const std::map<std::string_view, std::map<int, std::string_view>>
      typeSize;
  static const std::map<unsigned, PcodeOpType> opTypes;

  // visitor class
  class RetypdGeneratorVisitor
      : public llvm::InstVisitor<RetypdGeneratorVisitor> {
    RetypdGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    RetypdGeneratorVisitor(RetypdGenerator &cg,
                           std::vector<std::string> &constrains)
        : cg(cg) {}
    // overloaded visit functions
    void visitCastInst(CastInst &I);
    void visitCallBase(CallBase &I);
    void visitReturnInst(ReturnInst &I);
    void visitPHINode(PHINode &I);
    void visitLoadInst(LoadInst &I);
    void visitStoreInst(StoreInst &I);
    // void visitAllocaInst(AllocaInst &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitICmpInst(ICmpInst &I);
    void visitFCmpInst(FCmpInst &I);
    void visitSelectInst(SelectInst &I);
    // use opTypes to handle other insts.
    void visitInstruction(Instruction &I);
    void visitUnreachableInst(UnreachableInst &I) {}
    void visitBranchInst(BranchInst &I) {}
    void handlePHINodes();
  };
};

} // namespace notdec

#endif
