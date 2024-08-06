#include "TypeRecovery/StorageShapeGraph.h"
#include <cctype>
#include <queue>

namespace notdec::retypd {

void StorageShapeGraph::solve() {
  // write a worklist algorithm.
  std::queue<PNIConsNode *> Worklist;
  for (auto &C : Constraints) {
    Worklist.push(&C);
  }
  while (!Worklist.empty()) {
    PNIConsNode *C = Worklist.front();
    Worklist.pop();
    if ()
  }
}

// retypd. Figure 13.
// Lower case letters denote known integer or pointer types. Upper case letters
// denote inferred types. For example, the first column says that if X and Y are
// integral types in an ADD(X, Y ; Z) constraint, then Z is integral as well.
const char AddNodeCons::Rules[][3] = {
    {'i', 'i', 'I'}, {'I', 'I', 'i'}, {'p', 'I', 'P'},
    {'P', 'i', 'p'}, {'I', 'p', 'P'}, {'i', 'P', 'p'},
};

const char SubNodeCons::Rules[][3] = {
    {'i', 'I', 'I'}, {'I', 'i', 'i'}, {'P', 'i', 'p'}, {'P', 'p', 'I'},
    {'p', 'P', 'i'}, {'p', 'i', 'P'}, {'p', 'I', 'p'}};

std::pair<llvm::SmallVector<SSGNode *, 3>, bool> AddNodeCons::solve() {
  llvm::SmallVector<SSGNode *, 3> Changed;
  bool FullySolved = false;
  // first consider the alias case
  auto *Left = this->Left->PNIlookup();
  auto *Right = this->Right->PNIlookup();
  auto *Result = this->Result->PNIlookup();
  // Left alias right: Must be number
  // this includes the Left == Right == Result case.
  if (Left == Right) {
    bool IsChanged = Left->unifyStorageShape(Primitive{.name = "int"});
    if (IsChanged) {
      Changed.push_back(Left);
    }
    IsChanged = Right->unifyStorageShape(Primitive{.name = "int"});
    if (IsChanged) {
      Changed.push_back(Right);
    }
    IsChanged = Result->unifyStorageShape(Primitive{.name = "int"});
    if (IsChanged) {
      Changed.push_back(Result);
    }
    FullySolved = true;
  }
  // Left != Right.
  // If Left == Result, Right must be number, and Left and Result have same
  // type
  else if (Left == Result) {
    bool IsChanged = Right->unifyStorageShape(Primitive{.name = "int"});
    if (IsChanged) {
      Changed.push_back(Right);
    }
    FullySolved = !Left->isUnknown();
  } else if (Right == Result) {
    // same as above
    bool IsChanged = Left->unifyStorageShape(Primitive{.name = "int"});
    if (IsChanged) {
      Changed.push_back(Left);
    }
    FullySolved = !Right->isUnknown();
  } else {
    // no alias at all.
    // first scan lower case letters
    // if not match, continue.
    // if match, apply upper case letter constraints.
    // Update changed list accordingly.
    for (const char *Rule : Rules) {
      bool NotMatch = false;
      SSGNode *Arr[3] = {Left, Right, Result};
      for (unsigned i = 0; i < 3; i++) {
        if (std::islower(Rule[i]) && Rule[i] != Arr[i]->getIPChar()) {
          NotMatch = true;
          break;
        }
      }
      if (NotMatch) {
        continue;
      }
      // this rule match.
      for (unsigned i = 0; i < 3; i++) {
        if (std::isupper(Rule[i])) {
          StorageShapeTy ToUnify = fromIPChar(Rule[i]);
          bool IsChanged = Arr[i]->unifyStorageShape(ToUnify);
          if (IsChanged) {
            Changed.push_back(Arr[i]);
          }
        }
      }
    }
    FullySolved =
        !Left->isUnknown() && !Right->isUnknown() && !Result->isUnknown();
  }
  return {Changed, FullySolved};
}

StorageShapeTy fromIPChar(char C) {
  switch (C) {
  case 'I':
    return Primitive{.name = "int"};
  case 'P':
    return Pointer();
  default:
    assert(false && "fromIPChar: unhandled char");
  }
}

llvm::iplist<SSGNode>::iterator SSGNode::eraseFromParent() {
  auto &List =
      getParent()->*(StorageShapeGraph::getSublistAccess((SSGNode *)nullptr));
  return List.erase(getIterator());
}

// llvm::iplist<SSGLink>::iterator SSGLink::eraseFromParent() {
//   auto &List =
//       getParent()->*(StorageShapeGraph::getSublistAccess((SSGLink
//       *)nullptr));
//   return List.erase(getIterator());
// }

Primitive unifyPrimitive(const Primitive &Left, const Primitive &Right) {
  assert(Left == Right && "unifyPrimitive: Left and Right are different");
  return Left;
}

Pointer unifyPointer(const Pointer &Left, const Pointer &Right) {
  // TODO
  return Left;
}

// Like a operator
StorageShapeTy unify(const StorageShapeTy &Left, const StorageShapeTy &Right) {
  StorageShapeTy ret = Left;
  if (Left.index() == Right.index()) { // same inner type
    if (std::holds_alternative<Pointer>(Left)) {
      return unifyPointer(std::get<Pointer>(Left), std::get<Pointer>(Right));
    } else if (std::holds_alternative<Primitive>(Left)) {
      return unifyPrimitive(std::get<Primitive>(Left),
                            std::get<Primitive>(Right));
    } else if (std::holds_alternative<Unknown>(Left)) {
      return Left;
    }
    assert(false && "unify: unhandled type");
  } else {
    unsigned char Val = UniTyMergeMap[Left.index()][Right.index()];
    if (Val != 0) {
      return Left;
    } else {
      return Right;
    }
  }
}

} // namespace notdec::retypd
