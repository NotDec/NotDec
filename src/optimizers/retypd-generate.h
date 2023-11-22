#ifndef _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_
#define _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <map>
#include <string>
#include <vector>

namespace notdec::optimizers {

using namespace llvm;

struct RetypdRunner : PassInfoMixin<RetypdRunner> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &);
};

struct RetypdGenerator {
  std::map<size_t, Value *> type_var2val;
  std::map<Value *, size_t> val2type_var;
  std::map<Function *, std::vector<std::string>> func_constrains;
  std::map<Function *, std::set<Function *>> call_graphs;
  std::string data_layout;
  unsigned pointer_size = 0;

  void run(Module &M);

protected:
  void run(Function &M);

public:
  void gen_call_graph(Module &M);
  void gen_json(std::string outputFilename);

  // id to type_var name
  size_t get_or_insert_value(Value *val);
  std::string get_func_name(Function &func);
  std::string get_type_var(Value *val);
  std::string deref_label(Value *val, long size, const char *mode);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Instruction *I) {
    return I->getOpcode() == Instruction::CastOps::BitCast ||
           I->getOpcode() == Instruction::CastOps::AddrSpaceCast ||
           I->getOpcode() == Instruction::CastOps::IntToPtr ||
           I->getOpcode() == Instruction::CastOps::PtrToInt;
  }
  static std::string deref(std::string label, const char *mode, long size,
                           int offset, int count = 0);
  static std::string ret2name(std::string funcName);
  static std::string id2name(size_t id);
  static std::string arg2name(std::string funcName, unsigned ind);
  static std::string sanitize_name(std::string s);

protected:
  size_t type_val_id = 0;

  struct PcodeOpType {
    const char *output;
    // allow for uniform initializations
    const char **inputs;
    int size;

  public:
    PcodeOpType(const char *output, const char *inputs[], int size)
        : output(output), inputs(inputs), size(size) {}
    void addConstrains(Instruction *I, std::vector<std::string> &constrains,
                       RetypdGenerator &cg) const;
  };

  static const std::map<std::string_view, std::map<int, std::string_view>>
      typeSize;
  static const std::map<unsigned, PcodeOpType> opTypes;

  // visitor class
  class RetypdGeneratorVisitor
      : public llvm::InstVisitor<RetypdGeneratorVisitor> {
    RetypdGenerator &cg;
    // constrain vector of current func.
    std::vector<std::string> &constrains;

  public:
    RetypdGeneratorVisitor(RetypdGenerator &cg,
                           std::vector<std::string> &constrains)
        : cg(cg), constrains(constrains) {}
    // overloaded visit functions
    void visitCastInst(CastInst &I);
    void visitCallBase(CallBase &I);
    void visitReturnInst(ReturnInst &I);
    void visitPHINode(PHINode &I);
    void visitLoadInst(LoadInst &I);
    void visitStoreInst(StoreInst &I);
    // void visitAllocaInst(AllocaInst &I);
    // void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitCmpInst(CmpInst &I);
    void visitSelectInst(SelectInst &I);
    // use opTypes to handle other insts.
    void visitInstruction(Instruction &I);
  };
};

} // namespace notdec::optimizers

#endif
