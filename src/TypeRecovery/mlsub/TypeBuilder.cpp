#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec/Utils/Utils.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#define DEBUG_TYPE "mlsub_typebuilder"

namespace notdec::mlsub {

using notdec::SimpleRange;
using notdec::ValueNamer;
using notdec::ast::FieldDecl;
using notdec::ast::HType;
using notdec::ast::RecordDecl;
using notdec::ast::TypedefDecl;
using notdec::ast::UnionDecl;

using binarysub::UBot;
using binarysub::UFunctionType;
using binarysub::UInter;
using binarysub::UPointerType;
using binarysub::UPrimitiveType;
using binarysub::URecordType;
using binarysub::URecursiveType;
using binarysub::UTop;
using binarysub::UType;
using binarysub::UTypePtr;
using binarysub::UTypeVariable;
using binarysub::UUnion;

using FieldEntry = std::pair<SimpleRange, HType *>;

TypeBuilder::TypeBuilder(TypeBuilderContext &Parent)
    : Parent(Parent), Ctx(Parent.Ctx) {
  initializeStructMergeInfo();
}

void TypeBuilder::setDebugRootLabel(std::optional<std::string> Label) {
  CurrentRootDebugLabel = std::move(Label);
  CurrentDebugPath.clear();
}

void TypeBuilder::pushDebugPath(std::string Frame) {
  CurrentDebugPath.push_back(std::move(Frame));
}

void TypeBuilder::popDebugPath() {
  assert(!CurrentDebugPath.empty() && "debug path underflow");
  CurrentDebugPath.pop_back();
}

HType *TypeBuilder::getVoidPtr() {
  return Ctx.getPointerType(false, Parent.PointerSize * 8, nullptr);
}

HType *TypeBuilder::getTopType(std::uint32_t BitSize) {
  return Ctx.getTopType(false, BitSize);
}

HType *TypeBuilder::getBottomType(std::uint32_t BitSize) {
  return Ctx.getBottomType(false, BitSize);
}

namespace {

constexpr llvm::StringLiteral kTraceConvertStructEnv =
    "NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT";
constexpr llvm::StringLiteral kTraceConvertStructLogFile =
    "04-typebuilder-convertstruct.log";

bool envFlagEnabled(llvm::StringRef Name) {
  auto *Value = std::getenv(Name.data());
  return Value != nullptr && Value[0] != '\0' && Value[0] != '0';
}

bool shouldTraceConvertStruct() {
  if (envFlagEnabled(kTraceConvertStructEnv)) {
    return true;
  }
  return ::llvm::DebugFlag && ::llvm::isCurrentDebugType(DEBUG_TYPE);
}

void emitConvertStructTrace(llvm::StringRef Content) {
  static bool ResetLogFile = false;
  if (auto WorkDir = notdec::getWorkDirOpt()) {
    if (!ResetLogFile) {
      if (std::error_code EC = llvm::sys::fs::create_directories(*WorkDir)) {
        llvm::dbgs() << Content;
        return;
      }
      auto TraceLogPath = notdec::join(*WorkDir, kTraceConvertStructLogFile.str());
      if (std::error_code EC = llvm::sys::fs::remove(TraceLogPath);
          EC && EC != std::errc::no_such_file_or_directory) {
        llvm::dbgs() << "Warning: cannot reset(rm) "
                     << kTraceConvertStructLogFile << " in " << *WorkDir
                     << ": " << EC.message() << "\n";
        llvm::dbgs() << Content;
        return;
      }
      ResetLogFile = true;
    }
    notdec::appendWorkDirLog(kTraceConvertStructLogFile, Content);
    return;
  }
  llvm::dbgs() << Content;
}

void appendCurrentRootLabel(llvm::raw_ostream &OS,
                            const std::optional<std::string> &Label) {
  if (Label) {
    OS << *Label;
    return;
  }
  OS << "<unknown>";
}

void appendCurrentDebugContext(
    llvm::raw_ostream &OS, const std::optional<std::string> &RootLabel,
    const std::vector<std::string> &DebugPath) {
  appendCurrentRootLabel(OS, RootLabel);
  if (DebugPath.empty()) {
    return;
  }
  OS << " path=";
  for (size_t I = 0; I < DebugPath.size(); ++I) {
    if (I != 0) {
      OS << " -> ";
    }
    OS << DebugPath[I];
  }
}

void appendCurrentDebugPathOnly(llvm::raw_ostream &OS,
                                const std::vector<std::string> &DebugPath) {
  if (DebugPath.empty()) {
    return;
  }
  OS << "path=";
  for (size_t I = 0; I < DebugPath.size(); ++I) {
    if (I != 0) {
      OS << " -> ";
    }
    OS << DebugPath[I];
  }
}

ast::TypedDecl *findRecursiveAnchorDecl(HType *Ty) {
  if (Ty == nullptr) {
    return nullptr;
  }
  if (auto *Decl = Ty->getAsRecordOrUnionDecl()) {
    return Decl;
  }
  // URecordType 的 HType 形态是 struct*；递归打印和降级要锚到 pointee。
  if (Ty->isPointerType()) {
    auto *Pointee = Ty->getPointeeType();
    return Pointee == nullptr ? nullptr : Pointee->getAsRecordOrUnionDecl();
  }
  // 递归 body 常见形态是 bottom/top 与 struct* 的交并。anchor 只需要那个
  // 具体聚合 decl，不能因为外层 set 就退回单字段 fallback 壳。
  if (auto *Set = llvm::dyn_cast<ast::SetUnionType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      if (auto *Decl = findRecursiveAnchorDecl(Term)) {
        return Decl;
      }
    }
    return nullptr;
  }
  if (auto *Set = llvm::dyn_cast<ast::SetInterType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      if (auto *Decl = findRecursiveAnchorDecl(Term)) {
        return Decl;
      }
    }
    return nullptr;
  }
  return nullptr;
}

struct ConvertStructTraceDepthScope {
  unsigned &Depth;

  explicit ConvertStructTraceDepthScope(unsigned &Depth) : Depth(Depth) {
    ++Depth;
  }

  ~ConvertStructTraceDepthScope() { --Depth; }
};

struct DebugPathScope {
  TypeBuilder &TB;
  bool Active = false;

  DebugPathScope(TypeBuilder &TB, std::string Frame)
      : TB(TB), Active(!Frame.empty()) {
    if (Active) {
      TB.pushDebugPath(std::move(Frame));
    }
  }

  ~DebugPathScope() {
    if (Active) {
      TB.popDebugPath();
    }
  }
};

struct StructMergeGroupScope {
  std::optional<std::uint32_t> &Slot;
  std::optional<std::uint32_t> Saved;

  StructMergeGroupScope(std::optional<std::uint32_t> &Slot,
                        std::optional<std::uint32_t> Value)
      : Slot(Slot), Saved(Slot) {
    if (Value) {
      Slot = Value;
    }
  }

  ~StructMergeGroupScope() { Slot = Saved; }
};

struct DisableStructMergeGroupScope {
  std::optional<std::uint32_t> &Slot;
  std::optional<std::uint32_t> Saved;

  explicit DisableStructMergeGroupScope(
      std::optional<std::uint32_t> &Slot)
      : Slot(Slot), Saved(Slot) {
    Slot.reset();
  }

  ~DisableStructMergeGroupScope() { Slot = Saved; }
};

// binarysub may materialize empty records as internal placeholders while it is
// normalizing recursive or compound shapes. They should stay transparent to the
// final HType layer instead of surfacing as user-visible empty aggregates.
bool isZeroSizedRecordMarker(const binarysub::UTypePtr &Ty) {
  if (Ty == nullptr) {
    return false;
  }
  if (auto *Record = std::get_if<binarysub::URecordType>(&Ty->v)) {
    if (Record->fields.empty()) {
      return true;
    }
    return std::all_of(Record->fields.begin(), Record->fields.end(),
                       [](const auto &Field) {
                         return isZeroSizedRecordMarker(Field.second);
                       });
  }
  return false;
}

bool isVagueFieldBoundType(const HType *Ty) {
  return Ty != nullptr && (Ty->isTopType() || Ty->isBottomType());
}

HType *chooseDualPointerFieldValueTy(const ast::DualPointerType *Ty,
                                     bool IsCovariant) {
  HType *LoadTy = Ty->getLoadType();
  HType *StoreTy = Ty->getStoreType();
  assert((LoadTy != nullptr || StoreTy != nullptr) &&
         "dual pointer field must have at least one side");
  if (LoadTy == nullptr) {
    return StoreTy;
  }
  if (StoreTy == nullptr) {
    return LoadTy;
  }

  bool LoadVague = isVagueFieldBoundType(LoadTy);
  bool StoreVague = isVagueFieldBoundType(StoreTy);
  if (LoadVague != StoreVague) {
    return LoadVague ? StoreTy : LoadTy;
  }

  return IsCovariant ? StoreTy : LoadTy;
}

// UType 的 set 节点已经在上游按同类 flatten 成 vector，这里只需要按顺序
// 过滤掉 marker 项，再线性 reduce 成 HType set。
template <typename ConvertFn, typename MergeFn>
HType *convertVisibleSetTerms(const std::vector<UTypePtr> &Terms,
                              ConvertFn &&ConvertTerm,
                              MergeFn &&MergeTerms) {
  HType *Result = nullptr;
  for (const auto &Term : Terms) {
    if (isZeroSizedRecordMarker(Term)) {
      continue;
    }
    auto *Converted = ConvertTerm(Term);
    Result = Result == nullptr ? Converted : MergeTerms(Result, Converted);
  }
  if (Result != nullptr) {
    return Result;
  }
  assert(!Terms.empty() && "set node must contain at least one term");
  return ConvertTerm(Terms.front());
}

template <typename MeasureFn>
int64_t maxSetTermMetric(const std::vector<UTypePtr> &Terms, MeasureFn &&Measure) {
  assert(!Terms.empty() && "set node must contain at least one term");
  int64_t MaxValue = Measure(Terms.front());
  for (size_t I = 1; I < Terms.size(); ++I) {
    MaxValue = std::max(MaxValue, Measure(Terms[I]));
  }
  return MaxValue;
}

void collectStructMergeOriginIds(const UTypePtr &Ty,
                                 std::set<std::uint32_t> &Origins,
                                 std::set<const UType *> &Seen) {
  if (!Ty || !Seen.insert(Ty.get()).second) {
    return;
  }
  if (auto *Var = std::get_if<UTypeVariable>(&Ty->v)) {
    Origins.insert(Var->originIds.begin(), Var->originIds.end());
    return;
  }
  if (auto *Record = std::get_if<URecordType>(&Ty->v)) {
    for (const auto &Field : Record->fields) {
      collectStructMergeOriginIds(Field.second, Origins, Seen);
    }
    return;
  }
  if (auto *Ptr = std::get_if<UPointerType>(&Ty->v)) {
    collectStructMergeOriginIds(Ptr->load, Origins, Seen);
    collectStructMergeOriginIds(Ptr->store, Origins, Seen);
    return;
  }
  if (auto *Func = std::get_if<UFunctionType>(&Ty->v)) {
    for (const auto &Arg : Func->args) {
      collectStructMergeOriginIds(Arg, Origins, Seen);
    }
    collectStructMergeOriginIds(Func->result, Origins, Seen);
    return;
  }
  if (auto *Union = std::get_if<UUnion>(&Ty->v)) {
    for (const auto &Term : Union->types) {
      collectStructMergeOriginIds(Term, Origins, Seen);
    }
    return;
  }
  if (auto *Inter = std::get_if<UInter>(&Ty->v)) {
    for (const auto &Term : Inter->types) {
      collectStructMergeOriginIds(Term, Origins, Seen);
    }
    return;
  }
  if (auto *Rec = std::get_if<URecursiveType>(&Ty->v)) {
    collectStructMergeOriginIds(Rec->body, Origins, Seen);
  }
}

std::pair<HType *, SimpleRange>
cropFieldTypeToRange(notdec::ast::HTypeContext &Ctx, HType *HT,
                     notdec::OffsetTy OldSize, SimpleRange RelativeRange) {
  if (RelativeRange.Size <= 0) {
    return {nullptr, {.Start = RelativeRange.Start, .Size = 0}};
  }
  if (RelativeRange.Start == 0 && RelativeRange.Size == OldSize) {
    return {HT, RelativeRange};
  }

  if (auto *AT = HT->getAs<ast::ArrayType>()) {
    if (auto NumElements = AT->getNumElements();
        NumElements && *NumElements != 0 && OldSize > 0 &&
        OldSize % *NumElements == 0) {
      auto ElemSize = OldSize / static_cast<notdec::OffsetTy>(*NumElements);
      if (ElemSize > 0) {
        auto Start =
            ((RelativeRange.Start + ElemSize - 1) / ElemSize) * ElemSize;
        auto End = (RelativeRange.end() / ElemSize) * ElemSize;
        if (End > Start) {
          auto NewCount = (End - Start) / ElemSize;
          return {AT->withSize(Ctx, static_cast<unsigned>(NewCount)),
                  {.Start = Start, .Size = End - Start}};
        }
      }
    }
  }

  if (RelativeRange.Size == 1) {
    return {Ctx.getChar(), RelativeRange};
  }

  // Fall back to a byte blob when we cannot preserve the original aggregate
  // shape after clipping. This keeps the enclosing layout within bounds.
  return {Ctx.getArrayType(false, Ctx.getChar(),
                           static_cast<unsigned>(RelativeRange.Size)),
          RelativeRange};
}

char hexDigit(unsigned Value) {
  assert(Value < 16);
  return Value < 10 ? static_cast<char>('0' + Value)
                    : static_cast<char>('A' + (Value - 10));
}

// Semantic primitive canonical names may contain '.' and other separators that
// are not valid C typedef identifiers, so encode them into a stable alias.
std::string makeSemanticPrimitiveTypedefName(const std::string &CanonicalName) {
  std::string Result = "notdec_sem_";
  Result.reserve(Result.size() + CanonicalName.size() * 3);
  for (unsigned char Ch : CanonicalName) {
    if (std::isalnum(Ch) || Ch == '_') {
      Result.push_back(static_cast<char>(Ch));
      continue;
    }
    Result.push_back('_');
    Result.push_back(hexDigit((Ch >> 4) & 0xF));
    Result.push_back(hexDigit(Ch & 0xF));
  }
  return Result;
}

} // namespace

HType *TypeBuilder::parsePrimitiveName(const std::string &Name,
                                       std::uint32_t BitSize) {
  // Integer types: "i8", "i16", "i32", "i64"
  if (Name == "sint") {
    return Ctx.getIntegerType(false, BitSize, false);
  }

  // Unsigned integer types: "u8", "u16", "u32", "u64"
  if (Name == "uint") {
    return Ctx.getIntegerType(false, BitSize, true);
  }

  // Floating point types
  if (Name == "float") {
    return Ctx.getFloatType(false, BitSize);
  }
  if (Name == "double") {
    assert(BitSize == 64);
    return Ctx.getFloatType(false, BitSize);
  }

  // Char and bool
  if (Name == "char") {
    return Ctx.getChar();
  }
  if (Name == "bool") {
    return Ctx.getBool();
  }

  auto &Registry = binarysub::globalPrimitiveSemanticRegistry();
  if (const auto *Family = Registry.findFamilyByCanonicalName(Name)) {
    assert(Family->bits == BitSize &&
           "semantic primitive bit width must match registered family");
    HType *BaseType = parsePrimitiveName(Family->base, Family->bits);
    std::string TypedefName = makeSemanticPrimitiveTypedefName(Name);
    auto *ExistingDecl = Ctx.getDecl(TypedefName);
    TypedefDecl *Decl = nullptr;
    if (ExistingDecl != nullptr) {
      Decl = llvm::dyn_cast<TypedefDecl>(ExistingDecl);
      assert(Decl != nullptr &&
             "semantic primitive typedef name collided with non-typedef decl");
    } else {
      Decl = TypedefDecl::Create(Ctx, TypedefName, BaseType);
      Decl->setComment("semantic primitive lattice node: " + Name);
    }
    return Ctx.getTypedefType(false, Decl);
  }

  assert(false && "Unknown primitive type!");
}

std::optional<ast::RecordDecl *>
TypeBuilder::getStructOrNull(binarysub::UTypePtr Ty) {
  // Check cache first
  auto CacheIt = TypeCache.find(Ty);
  if (CacheIt == TypeCache.end()) {
    return std::nullopt;
  }
  auto *HTy = CacheIt->second;
  if (HTy->isRecordType()) {
    return HTy->getAsRecordDecl();
  }
  // URecordType 的普通转换结果是 struct*。结构体布局复用时仍要找到
  // pointee record，否则字段视角再次转换同一个 URecordType 会重复建 decl。
  if (HTy->isPointerType()) {
    auto *Pointee = HTy->getPointeeType();
    if (Pointee != nullptr && Pointee->isRecordType()) {
      return Pointee->getAsRecordDecl();
    }
  }
  return std::nullopt;
}

ast::RecordDecl *TypeBuilder::getOrCreateStruct(binarysub::UTypePtr Ty) {
  auto It = getStructOrNull(Ty);
  // create as struct type, if not in type cache
  if (!It.has_value()) {
    RecordDecl *Decl = RecordDecl::Create(Ctx, ValueNamer::getName("struct_"));
    HType *Ret = Ctx.getRecordType(false, Decl);
    TypeCache[Ty] = Ret;
    It = Decl;
  }
  return *It;
}

void TypeBuilder::initializeStructMergeInfo() {
  auto *Info = Parent.StructMerge;
  if (Info == nullptr) {
    return;
  }

  std::map<std::uint32_t, std::set<std::uint32_t>> CandidateLabels;
  for (const auto &Candidate : Info->candidates) {
    CandidateLabels[Candidate.id] = Candidate.labelIds;
  }

  for (const auto &Group : Info->groups) {
    StructMergeGroupCandidateCounts[Group.id] = Group.candidateIds.size();
    auto &Labels = StructMergeGroupLabels[Group.id];
    if (Group.labelId) {
      Labels.insert(*Group.labelId);
    }
    for (auto CandidateId : Group.candidateIds) {
      auto It = CandidateLabels.find(CandidateId);
      if (It == CandidateLabels.end()) {
        continue;
      }
      Labels.insert(It->second.begin(), It->second.end());
    }
  }
}

std::optional<std::uint32_t> TypeBuilder::findStructMergeGroupForSet(
    const std::vector<binarysub::UTypePtr> &Terms) {
  if (StructMergeGroupLabels.empty()) {
    return findStructMergeGroupForCurrentRoot();
  }

  std::set<std::uint32_t> Origins;
  for (const auto &Term : Terms) {
    if (auto *Var = std::get_if<UTypeVariable>(&Term->v)) {
      Origins.insert(Var->originIds.begin(), Var->originIds.end());
    }
  }
  if (Origins.empty()) {
    return std::nullopt;
  }
  return findStructMergeGroupForOrigins(Origins);
}

std::optional<std::uint32_t> TypeBuilder::findStructMergeGroupForOrigins(
    const std::set<std::uint32_t> &Origins) {
  if (StructMergeGroupLabels.empty()) {
    return findStructMergeGroupForCurrentRoot();
  }
  if (Origins.empty()) {
    return std::nullopt;
  }
  std::optional<std::uint32_t> BestGroup;
  std::size_t BestOverlap = 0;
  std::size_t BestCandidates = 0;
  for (const auto &[GroupId, Labels] : StructMergeGroupLabels) {
    std::size_t Overlap = 0;
    for (auto Origin : Origins) {
      if (Labels.count(Origin)) {
        ++Overlap;
      }
    }
    if (Overlap == 0) {
      continue;
    }

    auto CandidateCount = StructMergeGroupCandidateCounts[GroupId];
    if (!BestGroup || Overlap > BestOverlap ||
        (Overlap == BestOverlap && CandidateCount > BestCandidates)) {
      BestGroup = GroupId;
      BestOverlap = Overlap;
      BestCandidates = CandidateCount;
    }
  }
  if (BestGroup) {
    return BestGroup;
  }
  return findStructMergeGroupForCurrentRoot();
}

std::optional<std::uint32_t>
TypeBuilder::findStructMergeGroupForType(const binarysub::UTypePtr &Ty) {
  std::set<std::uint32_t> Origins;
  std::set<const UType *> Seen;
  collectStructMergeOriginIds(Ty, Origins, Seen);
  return findStructMergeGroupForOrigins(Origins);
}

std::optional<std::uint32_t> TypeBuilder::findStructMergeGroupForCurrentRoot() {
  auto *RootGroups = Parent.StructMergeRootGroups;
  if (RootGroups == nullptr || !CurrentRootDebugLabel ||
      !CurrentDebugPath.empty()) {
    return std::nullopt;
  }
  auto It = RootGroups->find(*CurrentRootDebugLabel);
  if (It == RootGroups->end() || It->second.empty()) {
    return std::nullopt;
  }

  std::optional<std::uint32_t> BestGroup;
  std::size_t BestCandidates = 0;
  for (auto GroupId : It->second) {
    auto CandidateCount = StructMergeGroupCandidateCounts[GroupId];
    if (!BestGroup || CandidateCount > BestCandidates) {
      BestGroup = GroupId;
      BestCandidates = CandidateCount;
    }
  }
  return BestGroup;
}

ast::RecordDecl *
TypeBuilder::getOrCreateStructMergeDecl(std::uint32_t GroupId) {
  auto It = StructMergeGroupDecls.find(GroupId);
  if (It != StructMergeGroupDecls.end()) {
    return It->second;
  }
  auto *Decl = RecordDecl::Create(Ctx, ValueNamer::getName("struct_"));
  StructMergeGroupDecls[GroupId] = Decl;
  return Decl;
}

void TypeBuilder::bindStructMergeDecl(binarysub::UTypePtr Ty,
                                      std::uint32_t GroupId) {
  if (TypeCache.find(Ty) != TypeCache.end()) {
    return;
  }
  auto *Decl = getOrCreateStructMergeDecl(GroupId);
  TypeCache[Ty] = Ctx.getRecordType(false, Decl);
}

TypeBuilder::RecordLayoutKey TypeBuilder::buildRecordLayoutKey(
    const std::vector<std::pair<SimpleRange, HType *>> &Fields,
    std::optional<SimpleRange> ValidRange) const {
  std::vector<std::tuple<OffsetTy, OffsetTy, HType *>> KeyFields;
  KeyFields.reserve(Fields.size());
  for (const auto &Field : Fields) {
    KeyFields.emplace_back(Field.first.Start, Field.first.Size,
                           Field.second == nullptr
                               ? nullptr
                               : Field.second->getCanonicalType());
  }
  std::sort(KeyFields.begin(), KeyFields.end());
  return {ValidRange, std::move(KeyFields)};
}

std::optional<ast::RecordDecl *> TypeBuilder::findExactRecordLayout(
    const std::vector<std::pair<SimpleRange, HType *>> &Fields,
    std::optional<SimpleRange> ValidRange) const {
  auto It = ExactRecordLayoutDecls.find(buildRecordLayoutKey(Fields, ValidRange));
  if (It == ExactRecordLayoutDecls.end()) {
    return std::nullopt;
  }
  return It->second;
}

void TypeBuilder::rememberExactRecordLayout(
    ast::RecordDecl *Decl,
    const std::vector<std::pair<SimpleRange, HType *>> &Fields,
    std::optional<SimpleRange> ValidRange) {
  if (Decl == nullptr || Fields.empty()) {
    return;
  }
  ExactRecordLayoutDecls.emplace(buildRecordLayoutKey(Fields, ValidRange), Decl);
}

HType *TypeBuilder::finalizeRecursiveType(const binarysub::UTypePtr &Ty,
                                          HType *Result) {
  auto ForceStructDecl = getStructOrNull(Ty);
  if (!ForceStructDecl) {
    return Result;
  }

  auto *Decl = ForceStructDecl.value();
  if (Result->isRecordType() && Result->getAsRecordDecl() == Decl) {
    return Result;
  }

  assert(Decl->getFields().empty());
  auto SizeBits = binarysub::get_size(Ty);
  auto SizeBytes = SizeBits == 0 ? 0 : (SizeBits + 7) / 8;
  if (SizeBytes == 0) {
    SizeBytes = Parent.PointerSize;
  }
  Decl->addField(ast::FieldDecl{
      .R = {.Start = 0, .Size = static_cast<OffsetTy>(SizeBytes)},
      .Type = Result,
      .Name = ValueNamer::getName("rec_"),
      .Comment = "at offset: 0",
  });
  return TypeCache.at(Ty);
}

HType *TypeBuilder::getFieldAddressValueTy(HType *FieldTy, bool IsCovariant) {
  assert(FieldTy != nullptr && "field type cannot be null");
  if (auto *DPT = llvm::dyn_cast<ast::DualPointerType>(FieldTy)) {
    return chooseDualPointerFieldValueTy(DPT, IsCovariant);
  }
  if (auto *Set = llvm::dyn_cast<ast::SetUnionType>(FieldTy)) {
    std::vector<HType *> Terms;
    for (auto *Term : Set->getTypes()) {
      Terms.push_back(getFieldAddressValueTy(Term, IsCovariant));
    }
    return Ctx.getSetUnionType(false, std::move(Terms));
  }
  if (auto *Set = llvm::dyn_cast<ast::SetInterType>(FieldTy)) {
    std::vector<HType *> Terms;
    for (auto *Term : Set->getTypes()) {
      Terms.push_back(getFieldAddressValueTy(Term, IsCovariant));
    }
    return Ctx.getSetInterType(false, std::move(Terms));
  }
  return FieldTy;
}

HType *TypeBuilder::convertFieldType(const binarysub::UTypePtr &Ty,
                                     std::optional<int64_t> FieldSizeBytes,
                                     bool IsCovariant) {
  auto getFallbackBitSize = [&]() -> std::uint32_t {
    if (FieldSizeBytes.has_value()) {
      return static_cast<std::uint32_t>(*FieldSizeBytes * 8);
    }
    return binarysub::get_size(Ty);
  };
  auto convertPointerSide = [&](const UTypePtr &Side) -> HType * {
    if (!Side) {
      return nullptr;
    }
    return convert(Side);
  };

  if (std::get_if<UTop>(&Ty->v)) {
    return getTopType(getFallbackBitSize());
  } else if (std::get_if<UBot>(&Ty->v)) {
    return getBottomType(getFallbackBitSize());
  } else if (auto *V = std::get_if<UPrimitiveType>(&Ty->v)) {
    return parsePrimitiveName(V->name, binarysub::get_size(Ty));
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    return convertVariable(*V);
  } else if (auto *V = std::get_if<UFunctionType>(&Ty->v)) {
    std::vector<HType *> Params;
    for (auto &P : V->args) {
      Params.push_back(convert(P));
    }
    std::vector<HType *> RetTypes;
    if (V->result) {
      RetTypes.push_back(convert(V->result));
    }
    auto FTy = Ctx.getFunctionType(false, RetTypes, Params);
    return getPtrTy(FTy);
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    HType *LoadTy = convertPointerSide(V->load);
    HType *StoreTy = convertPointerSide(V->store);
    auto *DPT = Ctx.getDualPointerType(false, V->psize, LoadTy, StoreTy);
    return getFieldAddressValueTy(DPT, IsCovariant);
  } else if (auto *PT = std::get_if<URecordType>(&Ty->v)) {
    auto &T = *PT;
    std::vector<std::pair<OffsetRange, UTypePtr>> RawFields;
    for (const auto &field : T.fields) {
      RawFields.push_back({OffsetRange::fromStr(field.first), field.second});
    }
    std::string PathFrame = "record(fields=" + std::to_string(T.fields.size()) +
                            ", field_bytes=" +
                            (FieldSizeBytes ? std::to_string(*FieldSizeBytes)
                                            : std::string("<none>")) +
                            ")";
    DebugPathScope PathScope(*this, std::move(PathFrame));
    return convertStruct(Ty, RawFields, FieldSizeBytes, false, IsCovariant);
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    StructMergeGroupScope StructMergeScope(ActiveStructMergeGroup,
                                           findStructMergeGroupForSet(V->types));
    return convertVisibleSetTerms(
        V->types,
        [&](const UTypePtr &Term) {
          return convertFieldType(Term, FieldSizeBytes, IsCovariant);
        },
        [&](HType *LhsTy, HType *RhsTy) { return doUnion(LhsTy, RhsTy); });
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    StructMergeGroupScope StructMergeScope(ActiveStructMergeGroup,
                                           findStructMergeGroupForSet(V->types));
    return convertVisibleSetTerms(
        V->types,
        [&](const UTypePtr &Term) {
          return convertFieldType(Term, FieldSizeBytes, IsCovariant);
        },
        [&](HType *LhsTy, HType *RhsTy) { return doInter(LhsTy, RhsTy); });
  } else if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    return convertRecursive(Ty, *V);
  }
  assert(false && "Unhandled field UType variant");
}

HType *TypeBuilder::convertRecursive(const binarysub::UTypePtr &Ty,
                                     const binarysub::URecursiveType &T) {
  auto SizeBits = binarysub::get_size(Ty);
  auto *Binder = Ctx.createRecursiveBinder(ValueNamer::getName("rec_"),
                                           SizeBits == 0
                                               ? std::optional<unsigned>()
                                               : std::optional<unsigned>(
                                                     static_cast<unsigned>(
                                                         SizeBits)));
  HType *Binding = Ctx.getRecursiveBindingType(false, Binder);
  TypeCache[Ty] = Binding;

  auto NameIt = RecursiveTypeNames.find(T.name);
  assert((NameIt == RecursiveTypeNames.end() || NameIt->second == Binder) &&
         "Recursive type name rebound to a different binder");
  RecursiveTypeNames[T.name] = Binder;

  auto [_, Inserted] = InProgress.insert(Ty);
  assert(Inserted && "Recursive type should not be re-entered before caching");

  HType *Body = convert(T.body);
  Binder->setBody(Body);

  InProgress.erase(Ty);
  RecursiveTypeNames.erase(T.name);

  if (auto *BodyDecl = findRecursiveAnchorDecl(Body)) {
    Binder->setAnchorDecl(BodyDecl);
    return Binding;
  }

  auto *AnchorDecl = RecordDecl::Create(Ctx, Binder->getName());
  Binder->setAnchorDecl(AnchorDecl);
  auto SizeBytes = SizeBits == 0 ? 0 : (SizeBits + 7) / 8;
  if (SizeBytes == 0) {
    SizeBytes = Parent.PointerSize;
  }
  AnchorDecl->addField(ast::FieldDecl{
      .R = {.Start = 0, .Size = static_cast<OffsetTy>(SizeBytes)},
      .Type = convertFieldType(T.body, SizeBytes),
      .Name = ValueNamer::getName("field_"),
      .Comment = "recursive body at offset: 0",
  });
  return Binding;
}

HType *TypeBuilder::convert(UTypePtr Ty) {

  // Check cache first
  auto CacheIt = TypeCache.find(Ty);
  if (CacheIt != TypeCache.end()) {
    // convertStruct()/struct-merge 会把 URecordType 临时绑定到 RecordType，
    // 方便复用 decl。但 URecordType 对外仍是地址语义，cache hit 也要返回
    // struct*，不能泄漏这个内部对象视图。
    if (std::get_if<URecordType>(&Ty->v) && CacheIt->second->isRecordType()) {
      return getPtrTy(CacheIt->second);
    }
    return CacheIt->second;
  }

  if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    return convertRecursive(Ty, *V);
  }

  // Cycles must be anchored by an explicit URecursiveType.
  if (InProgress.count(Ty)) {
    llvm::errs() << "Unexpected recursive UType without URecursiveType: "
                 << binarysub::printType(Ty) << "\n";
    assert(false &&
           "Unexpected recursive UType without explicit URecursiveType");
  }

  InProgress.insert(Ty);

  HType *Result = nullptr;
  // 先处理非指针类型
  if (std::get_if<UTop>(&Ty->v)) {
    Result = getTopType(binarysub::get_size(Ty));
  } else if (std::get_if<UBot>(&Ty->v)) {
    Result = getBottomType(binarysub::get_size(Ty));
  } else if (auto *V = std::get_if<UPrimitiveType>(&Ty->v)) {
    // TODO 这里是不是应该用到ObjSize大小。而不是名字里面带大小。
    Result = parsePrimitiveName(V->name, binarysub::get_size(Ty));
  } else if (auto *V = std::get_if<UFunctionType>(&Ty->v)) {
    // 最后返回函数指针类型
    std::vector<HType *> Params;
    for (auto &P : V->args) {
      Params.push_back(convert(P));
    }
    std::vector<HType *> RetTypes;
    if (V->result) {
      RetTypes.push_back(convert(V->result));
    }
    auto FTy = Ctx.getFunctionType(false, RetTypes, Params);
    Result = getPtrTy(FTy);
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    Result = convertVariable(*V);
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    StructMergeGroupScope StructMergeScope(ActiveStructMergeGroup,
                                           findStructMergeGroupForSet(V->types));
    Result = convertVisibleSetTerms(
        V->types, [&](const UTypePtr &Term) { return convert(Term); },
        [&](HType *LhsTy, HType *RhsTy) { return doUnion(LhsTy, RhsTy); });
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    StructMergeGroupScope StructMergeScope(ActiveStructMergeGroup,
                                           findStructMergeGroupForSet(V->types));
    Result = convertVisibleSetTerms(
        V->types, [&](const UTypePtr &Term) { return convert(Term); },
        [&](HType *LhsTy, HType *RhsTy) { return doInter(LhsTy, RhsTy); });
  } else if (std::get_if<UPointerType>(&Ty->v)) {
    Result = convertPointer(Ty);
  } else if (std::get_if<URecordType>(&Ty->v)) {
    Result = convertPointer(Ty);
  } else {
    assert(false && "Unhandled UType variant");
  }

  InProgress.erase(Ty);
  TypeCache[Ty] = Result;
  return Result;
}

int64_t TypeBuilder::accessedPointeeSizeInBits(const binarysub::UTypePtr &Ty) {
  if (auto *V = std::get_if<UTop>(&Ty->v)) {
    return V->size;
  } else if (auto *V = std::get_if<UBot>(&Ty->v)) {
    return V->size;
  } else if (auto *V = std::get_if<UPrimitiveType>(&Ty->v)) {
    return V->size;
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    // Stop recursive at the pointer type.
    return V->psize;
  } else if (std::get_if<UFunctionType>(&Ty->v)) {
    return binarysub::get_size(Ty);
  } else if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    return accessedPointeeSizeInBits(V->body);
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    return V->size;
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    return maxSetTermMetric(
        V->types,
        [&](const UTypePtr &Term) { return accessedPointeeSizeInBits(Term); });
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    return maxSetTermMetric(
        V->types,
        [&](const UTypePtr &Term) { return accessedPointeeSizeInBits(Term); });
  } else if (auto *V = std::get_if<URecordType>(&Ty->v)) {
    auto MaxElem = 0;
    for (const auto &field : V->fields) {
      auto OR = OffsetRange::fromStr(field.first);
      auto PointeeSizeInBits = accessedPointeeSizeInBits(field.second);
      auto Size =
          OR.access.empty() ? OR.offset * 8 + PointeeSizeInBits
                            : OR.maxAccess() * 8;
      if (Size > MaxElem) {
        MaxElem = Size;
      }
    }
    return MaxElem;
  } else {
    assert(false && "Unhandled UType variant");
  }
}

// 根据要求的range创建结构体，按需插入padding。
// 1. 创建union panel的时候，创建匿名结构体。此时T==nullptr
// 2. 最后构造结构体的时候，按照range构造。此时T!=nullptr
// TypeCache 仍由外层转换流程维护，避免同一个 URecordType 在不同 PointeeSize
// 下被裁剪成不同布局时串缓存。
HType *TypeBuilder::craftStruct(const std::vector<FieldEntry> &Fields,
                                std::optional<SimpleRange> ValidRange,
                                std::optional<std::string> Name,
                                const binarysub::UTypePtr *T,
                                bool IsCovariant) {
  if (!Name) {
    Name = ValueNamer::getName("struct_");
  }
  RecordDecl *PrevDecl = nullptr;
  if (T) {
    PrevDecl = getStructOrNull(*T).value_or(nullptr);
  }
  RecordDecl *Decl = PrevDecl;
  if (Decl == nullptr) {
    if (auto Existing = findExactRecordLayout(Fields, ValidRange)) {
      return Ctx.getRecordType(false, *Existing);
    }
    // 临时的结构体
    Decl = ast::RecordDecl::Create(Ctx, Name.value());
  }
  assert(Decl != nullptr);

  auto FinishRecord = [&](RecordDecl *Record) -> HType * {
    auto *Ret = Ctx.getRecordType(false, Record);
    rememberExactRecordLayout(Record, Fields, ValidRange);
    return Ret;
  };

  if (PrevDecl != nullptr && !Decl->getFields().empty()) {
    auto ExistingFieldCount =
        std::count_if(Decl->getFields().begin(), Decl->getFields().end(),
                      [](const FieldDecl &Field) { return !Field.isPadding; });
    if (Fields.empty() ||
        Fields.size() <= static_cast<std::size_t>(ExistingFieldCount)) {
      return FinishRecord(Decl);
    }
    // A later candidate in the same struct-merge group can carry more fields.
    // Keep the shared decl identity but rebuild its layout from the richer
    // candidate.
    Decl->getFields().clear();
  }

  assert(Decl->getFields().empty());
  if (Fields.size() == 0) {
    if (!ValidRange) {
      assert(false);
    } else {
      // create a struct with only padding:
      auto FieldName = ValueNamer::getName("padding_");
      auto CurrentDecl = FieldDecl{
          .R = *ValidRange,
          .Type = Ctx.getArrayType(false, Ctx.getChar(), ValidRange->Size),
          .Name = FieldName,
          .Comment = "at offset: " + std::to_string(ValidRange->Start),
          .isPadding = true};
      Decl->addField(CurrentDecl);
      return FinishRecord(Decl);
    }
  }

  auto Current = Fields.front().first.Start;
  if (ValidRange) {
    Current = ValidRange->Start;
  }

  for (size_t i = 0; i < Fields.size(); i++) {
    auto &Ent = Fields[i];
    if (Ent.first.Size == 0) {
      llvm::errs() << "Warning: Skip zero sized field at offset: "
                   << Ent.first.Start << "\n";
      continue;
    }
    auto EffectiveRange = Ent.first;
    if (ValidRange) {
      auto IR = Ent.first.intersect(*ValidRange);
      if (IR.Size == 0) {
        continue;
      }
      EffectiveRange = IR;
    }

    HType *Ty = Ent.second;
    if (EffectiveRange != Ent.first) {
      auto Cropped = cropFieldTypeToRange(
          Ctx, Ty, Ent.first.Size,
          SimpleRange{.Start = EffectiveRange.Start - Ent.first.Start,
                      .Size = EffectiveRange.Size});
      if (Cropped.first == nullptr || Cropped.second.Size == 0) {
        continue;
      }
      EffectiveRange = {.Start = Ent.first.Start + Cropped.second.Start,
                        .Size = Cropped.second.Size};
      Ty = Cropped.first;
    }

    auto FieldName = ValueNamer::getName("field_");
    auto CurrentDecl =
        FieldDecl{.R = EffectiveRange,
                  .Type = Ty,
                  .Name = FieldName,
                  .Comment =
                      "at offset: " + std::to_string(EffectiveRange.Start)};
    std::optional<FieldDecl> PaddingAfter = std::nullopt;

    // Try to calc expand end:
    // if no ValidRange, no next ent: ent.end()
    // if no valid range, has next ent: max(next.start, ent.end())
    // if has valid range, no next ent: max(ValidRang, ent.end())
    // if has valid range, has next end: min(ValidRange, next.start)
    auto ExpandEnd = Ent.first.end();
    // if there is space to next field
    if (i + 1 < Fields.size()) {
      auto &Next = Fields.at(i + 1);
      ExpandEnd = std::max(ExpandEnd, Next.first.Start);
    } else {
      // if no next field, use valid range.
      if (ValidRange) {
        ExpandEnd = ValidRange->end();
      }
    }

    // if it is char array, merge with elem
    auto *ValueTy = Ty;
    if (ValueTy->isCharArrayType()) {
      auto j = i + 1;
      for (; j < Fields.size(); j++) {
        auto &EntJ = Fields[j];
        auto *ValueTyJ = EntJ.second;
        if (!ValueTyJ->isCharType()) {
          break;
        }
        // merge to prev array.
        ExpandEnd = EntJ.first.end();

        if (j + 1 < Fields.size()) {
          auto &NextJ = Fields.at(j + 1);
          ExpandEnd = std::max(ExpandEnd, NextJ.first.Start);
        } else {
          if (ValidRange) {
            ExpandEnd = ValidRange->end();
          }
        }
      }
      // adjust i to skip some field
      i = j - 1;
    }
    // must less than valid range.
    if (ValidRange) {
      ExpandEnd = std::min(ExpandEnd, ValidRange->end());
    }

    // Try to expand array size.
    ValueTy = CurrentDecl.Type;
    if (auto *ArrTy = ValueTy->getAs<ast::ArrayType>()) {
      if (auto NumElements = ArrTy->getNumElements();
          NumElements && *NumElements != 0) {
        // Keep unsized arrays unsized here. We can only expand arrays when an
        // existing element count gives us a reliable element size.
        auto ElemSize = CurrentDecl.R.Size / *NumElements;
        if (ElemSize > 0) {
          auto NewCount = (ExpandEnd - CurrentDecl.R.Start) / ElemSize;
          CurrentDecl.R.Size = NewCount * ElemSize;
          CurrentDecl.Type = ArrTy->withSize(Ctx, NewCount);
        }
      }
    }

    std::optional<FieldDecl> PaddingBefore = std::nullopt;
    // Add padding at the beginning
    if (Current < CurrentDecl.R.Start) {
      auto PaddingSize = CurrentDecl.R.Start - Current;
      PaddingBefore = FieldDecl{
          .R = SimpleRange{.Start = Current, .Size = PaddingSize},
          .Type = Ctx.getArrayType(false, Ctx.getChar(), PaddingSize),
          .Name = ValueNamer::getName("padding_"),
          .Comment = "at offset: " + std::to_string(Current),
          .isPadding = true,
      };
      Current = CurrentDecl.R.Start;
    }

    Current = CurrentDecl.R.end();

    // add padding after if there is space
    if (CurrentDecl.R.end() < ExpandEnd) {
      auto PaddingSize = ExpandEnd - CurrentDecl.R.end();
      PaddingAfter = FieldDecl{
          .R = SimpleRange{.Start = CurrentDecl.R.end(), .Size = PaddingSize},
          .Type = Ctx.getArrayType(false, Ctx.getChar(), PaddingSize),
          .Name = ValueNamer::getName("padding_"),
          .Comment = "at offset: " + std::to_string(CurrentDecl.R.end()),
          .isPadding = true,
      };
      Current = ExpandEnd;
    }
    if (PaddingBefore) {
      Decl->addField(*PaddingBefore);
    }
    Decl->addField(CurrentDecl);
    if (PaddingAfter) {
      Decl->addField(*PaddingAfter);
    }
  }
  if (Decl->getFields().size() == 0) {
    // add only padding
    if (ValidRange) {
      if (Current < ValidRange->end()) {
        auto PaddingSize = ValidRange->end() - Current;
        auto PaddingOnly = FieldDecl{
            .R = SimpleRange{.Start = Current, .Size = PaddingSize},
            .Type = Ctx.getArrayType(false, Ctx.getChar(), PaddingSize),
            .Name = ValueNamer::getName("padding_"),
            .Comment = "at offset: " + std::to_string(Current),
            .isPadding = true,
        };
        Current = ValidRange->end();
        Decl->addField(PaddingOnly);
      }
    }
  }

  if (!PrevDecl && Decl->getFields().size() == 0) {
    return getVoidPtr();
  }
  return FinishRecord(Decl);
}

HType *TypeBuilder::convertStruct(
    const binarysub::UTypePtr &T,
    std::vector<std::pair<OffsetRange, UTypePtr>> &RawFields,
    std::optional<int64_t> PointeeSize, bool PreferElementType,
    bool IsCovariant) {
  HType *Result = nullptr;
  auto StructMergeGroup = ActiveStructMergeGroup;
  if (!StructMergeGroup) {
    StructMergeGroup = findStructMergeGroupForType(T);
  }
  DisableStructMergeGroupScope DisableStructMergeScope(ActiveStructMergeGroup);
  if (StructMergeGroup) {
    bindStructMergeDecl(T, *StructMergeGroup);
  }
  auto TraceDepth = ConvertStructTraceDepth;
  ConvertStructTraceDepthScope TraceDepthScope(ConvertStructTraceDepth);

  if (shouldTraceConvertStruct()) {
    std::string Trace;
    llvm::raw_string_ostream OS(Trace);
    OS.indent(TraceDepth * 2);
    OS << "[TypeBuilder::convertStruct] begin ";
    if (TraceDepth == 0) {
      OS << "root=";
      appendCurrentDebugContext(OS, CurrentRootDebugLabel, CurrentDebugPath);
    } else {
      appendCurrentDebugPathOnly(OS, CurrentDebugPath);
    }
    OS << " pointee_size=";
    if (PointeeSize) {
      OS << *PointeeSize;
    } else {
      OS << "<none>";
    }
    OS << " raw_fields=" << RawFields.size() << "\n";
    for (size_t I = 0; I < RawFields.size(); ++I) {
      const auto &Ent = RawFields[I];
      int64_t AccessedBits = accessedPointeeSizeInBits(Ent.second);
      int64_t AccessedBytes = AccessedBits <= 0 ? 0 : (AccessedBits + 7) / 8;
      OS.indent((TraceDepth + 1) * 2);
      OS << "[" << I << "] range=" << Ent.first.str()
         << " offset=" << Ent.first.offset << " access=[";
      for (size_t J = 0; J < Ent.first.access.size(); ++J) {
        const auto &Access = Ent.first.access[J];
        if (J != 0) {
          OS << ", ";
        }
        OS << "{size=" << Access.Size << ", count=" << Access.Count << "}";
      }
      OS << "] accessed_bits=" << AccessedBits
         << " accessed_bytes=" << AccessedBytes
         << " utype=" << binarysub::printType(Ent.second) << "\n";
    }
    OS.flush();
    emitConvertStructTrace(Trace);
  }

  std::vector<FieldEntry> Fields;

  // 1 处理所有数组类型的成员
  // 提取所有乘数项
  std::set<int64_t> AllStrides;
  std::vector<std::pair<OffsetRange, UTypePtr>> RemainingEntries = RawFields;
  for (auto &E : RawFields) {
    for (auto &A : E.first.access) {
      if (A.Size > 0) {
        AllStrides.insert(A.Size);
      }
    }
  }

  while (!AllStrides.empty()) {
    // TODO 改成Vector
    auto MaxStride = *AllStrides.rbegin();
    assert(MaxStride > 0);
    AllStrides.erase(MaxStride);

    std::vector<std::pair<OffsetRange, UTypePtr>> HasStrideEntries;
    for (auto It = RemainingEntries.begin(); It != RemainingEntries.end();) {
      auto HasStride = std::any_of(
          It->first.access.begin(), It->first.access.end(),
          [MaxStride](const ArrayOffset &Access) {
            return Access.Size == MaxStride;
          });
      if (HasStride) {
        // Move matching entries out while keeping the iterator valid.
        HasStrideEntries.emplace_back(*It);
        It = RemainingEntries.erase(It);
        continue;
      }
      ++It;
    }

    // 按基址从小到大排序
    std::sort(HasStrideEntries.begin(), HasStrideEntries.end(),
              [](const std::pair<OffsetRange, UTypePtr> &E1,
                 const std::pair<OffsetRange, UTypePtr> &E2) {
                return E1.first.offset < E2.first.offset;
              });
    while (!HasStrideEntries.empty()) {
      auto &FrontOffset = HasStrideEntries.front().first;
      auto RangeStart = FrontOffset.offset;
      auto RangeEnd = FrontOffset.offset + MaxStride;
      auto FieldSize = MaxStride;
      if (PointeeSize) {
        if (RangeStart >= PointeeSize.value()) {
          HasStrideEntries.erase(HasStrideEntries.begin());
          continue;
        }
        if (RangeStart >= 0 && RangeEnd > PointeeSize.value()) {
          FieldSize = PointeeSize.value() - RangeStart;
          RangeEnd = RangeStart + FieldSize;
        }
      }
      if (FieldSize <= 0) {
        HasStrideEntries.erase(HasStrideEntries.begin());
        continue;
      }

      // 提取当前组中Base在当前范围内的模式 InRangeEnts
      auto InitialSize = HasStrideEntries.size();
      std::vector<std::pair<OffsetRange, UTypePtr>> InRangeEnts;
      std::optional<size_t> RemoveStart = std::nullopt;
      std::optional<size_t> RemoveEnd = HasStrideEntries.size();
      for (size_t I = 0; I < HasStrideEntries.size(); I++) {
        auto &Ent = HasStrideEntries.at(I);
        auto &CurrentOffset = Ent.first;
        if (CurrentOffset.offset >= RangeStart &&
            CurrentOffset.offset < RangeEnd) {
          InRangeEnts.push_back(Ent);
          if (!RemoveStart) {
            RemoveStart = I;
          }
        } else {
          RemoveEnd = I;
          break;
        }
      }

      assert(RemoveStart);
      HasStrideEntries.erase(HasStrideEntries.begin() + RemoveStart.value(),
                             HasStrideEntries.begin() + RemoveEnd.value());
      assert(InitialSize == (HasStrideEntries.size() + InRangeEnts.size()));

      // 去掉所有的最大stride，转换为子问题，递归处理，作为数组类型
      std::vector<std::pair<OffsetRange, UTypePtr>> SubProblem;
      for (auto Ent : InRangeEnts) {
        auto NewOffsetRange = Ent.first;
        NewOffsetRange.offset -= RangeStart;
        auto &AccArr = NewOffsetRange.access;
        AccArr.erase(std::remove(AccArr.begin(), AccArr.end(), MaxStride),
                     AccArr.end());
        SubProblem.push_back({NewOffsetRange, Ent.second});
      }
      // 因为是数组，必须得是这个大小
      std::string PathFrame = "stride(" + std::to_string(MaxStride) + ", " +
                              std::to_string(RangeStart) + ".." +
                              std::to_string(RangeEnd) + ")";
      DebugPathScope PathScope(*this, std::move(PathFrame));
      auto MemberTy =
          convertStruct(T, SubProblem, FieldSize, true, IsCovariant);
      if (FieldSize == MaxStride) {
        auto ArrTy = Ctx.getArrayType(false, MemberTy, std::nullopt);
        Fields.push_back({SimpleRange{.Start = RangeStart, .Size = MaxStride},
                          ArrTy});
      } else {
        Fields.push_back({SimpleRange{.Start = RangeStart, .Size = FieldSize},
                          MemberTy});
      }
    }
  }
  // 已经没有任何数组访问模式，接下来创建结构体和union类型
  if (RemainingEntries.size() > 0) {
    for (auto &Ent : RemainingEntries) {
      // Marker entries represent bookkeeping nodes, not material storage.
      if (isZeroSizedRecordMarker(Ent.second)) {
        continue;
      }
      // 肯定是非数组
      assert(Ent.first.access.empty());
      auto SizeInBits = accessedPointeeSizeInBits(Ent.second);
      auto Size = SizeInBits <= 0 ? 0 : (SizeInBits + 7) / 8;
      if (PointeeSize) {
        if (Ent.first.offset >= PointeeSize.value()) {
          continue;
        }
        if (Ent.first.offset >= 0 &&
            Ent.first.offset + Size > PointeeSize.value()) {
          Size = PointeeSize.value() - Ent.first.offset;
        }
      }
      if (Size <= 0) {
        continue;
      }
      std::string PathFrame = "field(" + Ent.first.str() + ", bytes=" +
                              std::to_string(Size) + ")";
      DebugPathScope PathScope(*this, std::move(PathFrame));
      Fields.push_back({SimpleRange{.Start = Ent.first.offset, .Size = Size},
                        convertFieldType(Ent.second, Size, IsCovariant)});
    }
  }

  // A marker can still collapse into a zero-sized range after normalization, so
  // prune such entries before overlap analysis and layout synthesis.
  Fields.erase(std::remove_if(Fields.begin(), Fields.end(),
                              [](const FieldEntry &F) {
                                return F.first.Size == 0;
                              }),
               Fields.end());

  if (PreferElementType && PointeeSize && *PointeeSize > 0) {
    if (Fields.empty()) {
      return Ctx.getArrayType(false, Ctx.getChar(),
                              static_cast<unsigned>(*PointeeSize));
    }
    if (Fields.size() == 1 && Fields.front().first.Start == 0 &&
        Fields.front().first.Size == *PointeeSize) {
      return Fields.front().second;
    }
  }

  auto IsOverlap = [](OffsetTy S1, OffsetTy E1, OffsetTy S2, OffsetTy E2) {
    assert(S1 < E1);
    assert(S2 < E2);
    if (std::max(S1, S2) < std::min(E1, E2)) {
      return true;
    }
    return false;
  };

  // Fields should not be mutated during the lifetime of ret vector
  auto FilterInRange =
      [&](const std::vector<std::pair<SimpleRange, HType *>> &Fields,
          OffsetTy Start, OffsetTy End) -> std::vector<size_t> {
    std::vector<size_t> Ret;
    for (size_t I = 0; I < Fields.size(); I++) {
      auto &F = Fields.at(I);
      auto FS = F.first.Start;
      auto FE = F.first.Size + FS;
      // overlaps
      if (IsOverlap(Start, End, FS, FE)) {
        Ret.push_back(I);
      }
    }
    return Ret;
  };

  while (true) {
    // 收集所有分割点
    std::set<OffsetTy> AllIndex;
    for (auto &F : Fields) {
      AllIndex.insert(F.first.Start);
      AllIndex.insert(F.first.end());
    }
    // 合并所有重叠：遍历所有最小范围区间，如果出现重叠则以此开始创建union类型。
    bool NoUpdate = true;
    for (auto It = AllIndex.begin(); It != AllIndex.end(); ++It) {
      auto NextIt = std::next(It);
      if (NextIt == AllIndex.end()) {
        break;
      }
      auto Start = *It;
      auto End = *NextIt;
      auto InRangeFieldIndex = FilterInRange(Fields, Start, End);
      if (InRangeFieldIndex.size() <= 1) {
        continue;
      }
      NoUpdate = false;

      // create a union here.
      decltype(InRangeFieldIndex.size()) OldSize = 1;
      auto NewSize = InRangeFieldIndex.size();
      while (
          NewSize >
          OldSize) { // 根据小的重叠区域，左右拓展找到需要处理创建union的所有Fields。
        OldSize = NewSize;
        for (auto Ind : InRangeFieldIndex) {
          auto &F = Fields.at(Ind);
          Start = std::min(Start, F.first.Start);
          End = std::max(End, F.first.end());
        }
        InRangeFieldIndex = FilterInRange(Fields, Start, End);
        NewSize = InRangeFieldIndex.size();
        assert(NewSize >= 2);
      }

      auto UnionStart = Start;
      auto UnionEnd = End;
      std::vector<FieldEntry> OtherFields;
      std::vector<FieldEntry> OverlapFields;
      for (auto &F : Fields) {
        if (IsOverlap(Start, End, F.first.Start, F.first.end())) {
          OverlapFields.push_back(FieldEntry{
              {.Start = F.first.Start - UnionStart, .Size = F.first.Size},
              F.second});
        } else {
          OtherFields.push_back(F);
        }
      }

      // #region build members using OverlapFields;
      // sort the entry by end offset.
      std::sort(OverlapFields.begin(), OverlapFields.end(),
                [](const FieldEntry &A, const FieldEntry &B) {
                  return A.first.end() < B.first.end();
                });
      // use std::min to find the min start offset.
      auto MinStartOff =
          std::min_element(OverlapFields.begin(), OverlapFields.end(),
                           [](const FieldEntry &A, const FieldEntry &B) {
                             return A.first.Start < B.first.Start;
                           })
              ->first.Start;
      auto MaxOff = OverlapFields.back().first.end();
      // unified start to 0
      assert(MinStartOff == 0);
      assert(MaxOff == (UnionEnd - UnionStart));
      // after determinize, there will not be nested struct. We assume
      // Offset
      // to struct == Min Start Offset. So set size as MaxOff - MinStartOff.
      auto OurSize = MaxOff - MinStartOff;
      std::vector<std::vector<FieldEntry>> UnionPanels;
      for (auto &F : OverlapFields) {
        bool inserted = false;
        for (auto &Panel : UnionPanels) {
          if (Panel.back().first.Start + Panel.back().first.Size <=
              F.first.Start) {
            Panel.push_back(F);
            inserted = true;
            break;
          }
        }
        if (!inserted) {
          UnionPanels.push_back({F});
        }
      }
      assert(UnionPanels.size() > 1);
      // create new node for each panel struct.
      std::vector<HType *> Members;
      for (auto &Panel : UnionPanels) {
        if (Panel.size() == 1) {
          // One overlapped field can be used as a union member directly. It is
          // already in final member shape.
          Members.push_back(Panel.front().second);
          continue;
        }
        // directly create a struct here
        std::string Name = ValueNamer::getName("Us_");
        auto E1 = craftStruct(Panel, SimpleRange{.Start = 0, .Size = OurSize},
                              Name, nullptr, IsCovariant);
        Members.push_back(E1);
      }
      if (Members.empty()) {
        llvm::errs() << "Warning: Empty union!\n";
      }
      // create union
      auto Name = ValueNamer::getName("union_");
      auto Decl = UnionDecl::Create(Ctx, Name);
      for (auto Ent : Members) {
        auto FieldName = ValueNamer::getName("field_");
        // Union需要起始大小是0，然后每一项大小都是OurSize。
        Decl->addMember(ast::FieldDecl{.R = {.Start = 0, .Size = OurSize},
                                       .Type = Ent,
                                       .Name = FieldName,
                                       .Comment = "at offset: 0"});
      }
      // push the merged union back to fields, and iterate again
      OtherFields.push_back({FieldEntry{
          SimpleRange{.Start = UnionStart + MinStartOff, .Size = OurSize},
          Ctx.getUnionType(false, Decl)}});
      assert(NoUpdate == false);
      // #endregion build members using OverlapFields;
      // reiterate with merged fields.
      Fields = OtherFields;
      break;
    }
    // no overlap, cannot create unions
    if (NoUpdate) {
      break;
    }
  } // end of while true
  // Now there is no overlap, create struct for Fields.
  if (Fields.empty()) {
    // Keep a sized shell when the pointee extent is known, otherwise callers
    // would lose the aggregate boundary and degrade to void* too early.
    if (PointeeSize && *PointeeSize > 0) {
      return craftStruct(
          {}, SimpleRange{.Start = 0, .Size = *PointeeSize}, std::nullopt, &T,
          IsCovariant);
    }
    return getVoidPtr();
  }
  // sort the entry by start offset.
  std::sort(Fields.begin(), Fields.end(),
            [](const FieldEntry &A, const FieldEntry &B) {
              return A.first.Start < B.first.Start;
            });
  auto MaxEndOff =
      std::max_element(Fields.begin(), Fields.end(),
                       [](const FieldEntry &A, const FieldEntry &B) {
                         return A.first.end() < B.first.end();
                       })
          ->first.end();
  // TODO
  // 是否会在递归的时候，要求当前大小？下面的大小就当做不知道多大的时候猜测出来的值。
  auto Size = MaxEndOff - Fields.front().first.Start;
  assert(Size >= 0);
  if (PointeeSize) {
    if (PointeeSize.value() < Size) {
      if (shouldTraceConvertStruct()) {
        std::string Trace;
        llvm::raw_string_ostream OS(Trace);
        OS.indent(TraceDepth * 2);
        OS << "[TypeBuilder::convertStruct] pointee/layout mismatch ";
        if (TraceDepth == 0) {
          OS << "root=";
          appendCurrentDebugContext(OS, CurrentRootDebugLabel,
                                    CurrentDebugPath);
        } else {
          appendCurrentDebugPathOnly(OS, CurrentDebugPath);
        }
        OS << " pointee_size=" << PointeeSize.value()
           << " synthesized_size=" << Size
           << " first_field_start=" << Fields.front().first.Start
           << " first_field_size=" << Fields.front().first.Size
           << " field_count=" << Fields.size() << "\n";
        OS.flush();
        emitConvertStructTrace(Trace);
      }
      Size = PointeeSize.value();
    }
    if (PointeeSize.value() > Size) {
      Size = PointeeSize.value();
    }
  }
  if (Fields.empty()) {
    llvm::errs() << "Warning: Empty struct!\n";
  }

  Result = craftStruct(
      Fields, SimpleRange{.Start = Fields.front().first.Start, .Size = Size},
      std::nullopt, &T, IsCovariant);
  return Result;
}

HType *TypeBuilder::convertPointer(const binarysub::UTypePtr &Ty,
                                   std::optional<int64_t> PointeeSize) {

  HType *Ret = nullptr;
  if (std::holds_alternative<UBot>(Ty->v) ||
      std::holds_alternative<UTop>(Ty->v)) {
    return getVoidPtr();
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    auto convertPointerSide = [&](const UTypePtr &Side) -> HType * {
      if (!Side) {
        return nullptr;
      }
      if (std::get_if<UTop>(&Side->v) || std::get_if<UBot>(&Side->v)) {
        return nullptr;
      }
      return convert(Side);
    };
    HType *LoadTy = convertPointerSide(V->load);
    HType *StoreTy = convertPointerSide(V->store);
    Ret = Ctx.getDualPointerType(false, V->psize, LoadTy, StoreTy);
  } else if (auto *PT = std::get_if<URecordType>(&Ty->v)) {
    auto &T = *PT;
    if (T.fields.empty()) {
      return getVoidPtr();
    }

    // 把field字符串都实例化为OffsetRange。
    std::vector<std::pair<OffsetRange, UTypePtr>> RawFields;
    for (const auto &field : T.fields) {
      RawFields.push_back({OffsetRange::fromStr(field.first), field.second});
    }
    std::string PathFrame =
        "pointer-record(fields=" + std::to_string(T.fields.size()) +
        ", pointee_size=" +
        (PointeeSize ? std::to_string(*PointeeSize) : std::string("<none>")) +
        ")";
    DebugPathScope PathScope(*this, std::move(PathFrame));
    Ret = getPtrTy(convertStruct(Ty, RawFields, PointeeSize));
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    StructMergeGroupScope StructMergeScope(ActiveStructMergeGroup,
                                           findStructMergeGroupForSet(V->types));
    Ret = convertVisibleSetTerms(
        V->types, [&](const UTypePtr &Term) { return convert(Term); },
        [&](HType *LhsTy, HType *RhsTy) { return doUnion(LhsTy, RhsTy); });
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    StructMergeGroupScope StructMergeScope(ActiveStructMergeGroup,
                                           findStructMergeGroupForSet(V->types));
    Ret = convertVisibleSetTerms(
        V->types, [&](const UTypePtr &Term) { return convert(Term); },
        [&](HType *LhsTy, HType *RhsTy) { return doInter(LhsTy, RhsTy); });
  } else {
    assert(false && "Unhandled Pointer UType variant");
  }
  assert((Ret->isPointerType() || Ret->isDualPointerType() ||
          Ret->isSetUnionType() || Ret->isSetInterType()) &&
         "Pointer conversion must preserve address-like semantics");
  return Ret;
}

HType *TypeBuilder::doUnion(HType *LhsTy, HType *RhsTy) {
  // Keep obvious identities collapsed, but otherwise preserve the
  // set-theoretic union in HType instead of forcing an early backend-oriented
  // approximation.
  if (LhsTy == RhsTy) {
    return LhsTy;
  }
  if (LhsTy->getCanonicalType() == RhsTy->getCanonicalType()) {
    return LhsTy->getCanonicalType();
  }
  return Ctx.getSetUnionType(false, LhsTy, RhsTy);
}

HType *TypeBuilder::doInter(HType *LhsTy, HType *RhsTy) {
  // Same rationale as doUnion(): preserve semantic information unless the
  // intersection is trivially identical.
  if (LhsTy == RhsTy) {
    return LhsTy;
  }
  if (LhsTy->getCanonicalType() == RhsTy->getCanonicalType()) {
    return LhsTy->getCanonicalType();
  }
  return Ctx.getSetInterType(false, LhsTy, RhsTy);
}

HType *TypeBuilder::convertVariable(const binarysub::UTypeVariable &T) {
  // For type variables, check if we have a recursive binding
  auto It = RecursiveTypeNames.find(T.name);
  if (It != RecursiveTypeNames.end()) {
    return Ctx.getRecursiveRefType(false, It->second);
  }

  // Otherwise, create a TypeVariableType to preserve the semantic information
  return Ctx.getTypeVariableType(false, T.name, T.size);
}

} // namespace notdec::mlsub
