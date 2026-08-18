#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"

#include "binarysub/SimpleTypeSnapshot.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SHA256.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <sys/resource.h>

namespace notdec::mlsub {
namespace {

constexpr std::uint32_t kCheckpointFormatVersion = 1;
constexpr std::uint32_t kValueMappingFormatVersion = 1;
constexpr std::uint32_t kPNDiffStateFormatVersion = 1;
constexpr std::uint32_t kBinarysubStateABI = 1;
constexpr std::uint64_t kNullCheckpointId =
    std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t kMaxCheckpointEntries = 1ULL << 31;
constexpr std::uint64_t kMaxCheckpointStringBytes = 1ULL << 26;
constexpr std::array<char, 8> kValueMappingMagic = {'N', 'D', 'P', 'C',
                                                    'M', 'A', 'P', '\0'};
constexpr std::array<char, 8> kPNDiffStateMagic = {'N', 'D', 'P', 'C',
                                                   'P', 'N', 'D', '\0'};

constexpr llvm::StringLiteral kManifestFile("manifest.json");
constexpr llvm::StringLiteral kInputIRFile("input.ll");
constexpr llvm::StringLiteral kInputBitcodeFile("input.bc");
constexpr llvm::StringLiteral kSimpleTypeGraphFile("simple-type-graph.bin");
constexpr llvm::StringLiteral kValueMappingFile("value-mapping.bin");
constexpr llvm::StringLiteral kPNDiffStateFile("pndiff-state.bin");

class CheckpointFailure final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// Checkpoint timings are process-level because graph restore allocates through
// the shared binarysub context. CPU time therefore includes all participating
// threads, while peak RSS is the process high-water mark at phase completion.
struct CheckpointTiming {
  std::chrono::steady_clock::time_point wallStart;
  std::uint64_t cpuStartUs = 0;
  bool cpuAvailable = false;
};

std::uint64_t timevalToMicroseconds(const timeval &value) {
  return static_cast<std::uint64_t>(value.tv_sec) * 1000000ULL +
         static_cast<std::uint64_t>(value.tv_usec);
}

CheckpointTiming startCheckpointTiming() {
  CheckpointTiming timing{.wallStart = std::chrono::steady_clock::now()};
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    timing.cpuStartUs = timevalToMicroseconds(usage.ru_utime) +
                        timevalToMicroseconds(usage.ru_stime);
    timing.cpuAvailable = true;
  }
  return timing;
}

void printCheckpointTiming(const CheckpointTiming &timing) {
  const auto wallMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - timing.wallStart)
                          .count();
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    llvm::errs() << " wall_ms=" << wallMs
                 << " cpu_ms=unavailable peak_rss_kib=unavailable";
    return;
  }
  const auto cpuEndUs = timevalToMicroseconds(usage.ru_utime) +
                        timevalToMicroseconds(usage.ru_stime);
  llvm::errs() << " wall_ms=" << wallMs << " cpu_ms=";
  if (timing.cpuAvailable && cpuEndUs >= timing.cpuStartUs) {
    llvm::errs() << (cpuEndUs - timing.cpuStartUs) / 1000ULL;
  } else {
    llvm::errs() << "unavailable";
  }
#if defined(__APPLE__)
  llvm::errs() << " peak_rss_kib=" << usage.ru_maxrss / 1024;
#else
  llvm::errs() << " peak_rss_kib=" << usage.ru_maxrss;
#endif
}

[[noreturn]] void failCheckpoint(const std::string &message) {
  throw CheckpointFailure(message);
}

std::string checkpointPath(llvm::StringRef directory, llvm::StringRef file) {
  llvm::SmallString<256> path(directory);
  llvm::sys::path::append(path, file);
  return path.str().str();
}

std::string sha256(llvm::StringRef content) {
  llvm::SHA256 hash;
  hash.update(content);
  return llvm::toHex(hash.final(), true);
}

std::string renderModule(const llvm::Module &module) {
  std::string text;
  llvm::raw_string_ostream output(text);
  module.print(output, nullptr);
  output.flush();
  return text;
}

std::string renderType(llvm::Type *type) {
  if (type == nullptr) {
    return "<null-type>";
  }
  std::string text;
  llvm::raw_string_ostream output(text);
  type->print(output);
  output.flush();
  return text;
}

std::string readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path, false);
  if (!buffer) {
    failCheckpoint("cannot read " + path.str() + ": " +
                   buffer.getError().message());
  }
  return (*buffer)->getBuffer().str();
}

void writeTextFile(llvm::StringRef path, llvm::StringRef content) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error, llvm::sys::fs::OF_Text);
  if (error) {
    failCheckpoint("cannot write " + path.str() + ": " + error.message());
  }
  output << content;
  output.flush();
  if (output.has_error()) {
    failCheckpoint("failed while writing " + path.str());
  }
}

void writeBitcodeFile(const llvm::Module &module, llvm::StringRef path) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error, llvm::sys::fs::OF_None);
  if (error) {
    failCheckpoint("cannot write " + path.str() + ": " + error.message());
  }
  llvm::WriteBitcodeToFile(module, output);
  output.flush();
  if (output.has_error()) {
    failCheckpoint("failed while writing " + path.str());
  }
}

std::string hashOptionalFileEnvironment(llvm::StringRef name) {
  const char *path = std::getenv(name.data());
  if (path == nullptr || *path == '\0') {
    return "<unset>";
  }
  return sha256(readFile(path));
}

std::string environmentValue(llvm::StringRef name) {
  const char *value = std::getenv(name.data());
  return value == nullptr ? "<unset>" : std::string(value);
}

enum class ValueSelectorKind : std::uint8_t {
  LLVMValue = 1,
  ReturnValue = 2,
  UConstant = 3,
  ConstantAddr = 4,
  StackObject = 5,
  HeapObject = 6,
};

// The stable text is human-readable and already used by SelectableValues.txt.
// `kind` and `type` remain separate so an accidental textual collision cannot
// turn one ExtValuePtr alternative into another during restore.
struct ValueSelector {
  ValueSelectorKind kind = ValueSelectorKind::LLVMValue;
  std::string stable;
  std::string type;

  bool operator<(const ValueSelector &other) const {
    return std::tie(kind, stable, type) <
           std::tie(other.kind, other.stable, other.type);
  }
  bool operator==(const ValueSelector &other) const {
    return kind == other.kind && stable == other.stable && type == other.type;
  }
};

ValueSelector makeValueSelector(const ExtValuePtr &value) {
  ValueSelector selector;
  selector.stable = toStableString(value);
  if (auto llvmValue = std::get_if<llvm::Value *>(&value)) {
    if (*llvmValue == nullptr) {
      failCheckpoint("cannot serialize a null LLVM Value");
    }
    selector.kind = ValueSelectorKind::LLVMValue;
    selector.type = renderType((*llvmValue)->getType());
  } else if (std::get_if<ReturnValue>(&value)) {
    selector.kind = ValueSelectorKind::ReturnValue;
    selector.type = renderType(getType(value));
  } else if (auto constant = std::get_if<UConstant>(&value)) {
    selector.kind = ValueSelectorKind::UConstant;
    selector.type = renderType(constant->Val->getType());
  } else if (auto address = std::get_if<ConstantAddr>(&value)) {
    selector.kind = ValueSelectorKind::ConstantAddr;
    selector.type = renderType(address->Val->getType());
  } else if (auto stack = std::get_if<StackObject>(&value)) {
    selector.kind = ValueSelectorKind::StackObject;
    selector.type = renderType(stack->Allocator->getAllocatedType());
  } else if (std::get_if<HeapObject>(&value)) {
    selector.kind = ValueSelectorKind::HeapObject;
    // HeapObject has no recoverable LLVM pointee type under opaque pointers.
    // Its allocator instruction is already part of the stable selector.
    selector.type = "<heap-object>";
  } else {
    failCheckpoint("unknown ExtValuePtr alternative");
  }
  return selector;
}

bool sameExtValue(const ExtValuePtr &left, const ExtValuePtr &right) {
  return !(left < right) && !(right < left);
}

// Intern every ExtValuePtr exactly once. The ordered selector table is the
// checkpoint-local ValueId namespace; runtime pointers remain process-local.
class ValueCatalog {
public:
  void add(const ExtValuePtr &value) {
    auto selector = makeValueSelector(value);
    auto [position, inserted] = Values.emplace(selector, value);
    if (!inserted && !sameExtValue(position->second, value)) {
      failCheckpoint("stable value selector collision: " + selector.stable);
    }
  }

  void finalize() {
    Selectors.clear();
    Ids.clear();
    Selectors.reserve(Values.size());
    std::uint64_t id = 0;
    for (const auto &[selector, value] : Values) {
      (void)value;
      Selectors.push_back(selector);
      Ids.emplace(selector, id++);
    }
  }

  std::uint64_t id(const ExtValuePtr &value) const {
    auto found = Ids.find(makeValueSelector(value));
    if (found == Ids.end()) {
      failCheckpoint("value was not interned before mapping serialization");
    }
    return found->second;
  }

  const std::vector<ValueSelector> &selectors() const { return Selectors; }

private:
  std::map<ValueSelector, ExtValuePtr> Values;
  std::map<ValueSelector, std::uint64_t> Ids;
  std::vector<ValueSelector> Selectors;
};

// Root IDs are shared by semantic mappings and the binarysub graph snapshot.
// Deduplicating them here preserves aliases between SCCs and global roots.
class RootCatalog {
public:
  std::uint64_t add(SimpleType type) {
    if (!type) {
      return kNullCheckpointId;
    }
    auto [position, inserted] = Ids.emplace(type.get(), Roots.size());
    if (inserted) {
      Roots.push_back(type);
    }
    return position->second;
  }

  const std::vector<SimpleType> &roots() const { return Roots; }

private:
  std::unordered_map<const binarysub::TypeNode *, std::uint64_t> Ids;
  std::vector<SimpleType> Roots;
};

// These records are the pointer-free semantic layer above the SimpleType graph.
// They are collected while LLVM pointers are valid, then encoded through the
// ValueCatalog IDs so restore can rebind them to a newly parsed Module.
struct SavedValueMapping {
  ExtValuePtr value;
  std::uint64_t root = kNullCheckpointId;
};

struct SavedOriginMapping {
  std::uint32_t variableId = 0;
  std::vector<ExtValuePtr> values;
};

struct SavedStore {
  ExtValuePtr address;
  ExtValuePtr source;
  std::uint32_t bitSize = 0;
};

// PNDiff nodes are saved by equivalence class rather than by their in-memory
// list address. Every value in one class is rebound to one restored PNINode;
// this preserves DSU semantics without serializing opaque PNIValue handles.
struct SavedPNDiffNode {
  unsigned long id = 0;
  std::string latticeType;
  bool conflict = false;
  std::vector<ExtValuePtr> values;
};

struct SavedPNDiffConstraint {
  bool isAdd = false;
  std::size_t sequence = 0;
  bool inWorklist = false;
  std::array<ExtValuePtr, 3> values;
  std::optional<ExtValuePtr> instruction;
};

struct SavedPNDiffState {
  std::size_t nextConstraintSequence = 0;
  std::vector<SavedPNDiffNode> nodes;
  std::vector<SavedPNDiffConstraint> constraints;
};

struct SavedSCC {
  std::string name;
  std::uint32_t level = 0;
  std::vector<std::string> functions;
  std::vector<SavedValueMapping> values;
  std::vector<ExtValuePtr> contraValues;
  std::vector<SavedOriginMapping> origins;
  std::vector<SavedStore> stores;
  SavedPNDiffState pndiff;
};

struct SavedSemanticState {
  std::uint64_t rootCount = 0;
  std::uint64_t memoryRoot = kNullCheckpointId;
  std::uint64_t storageRoot = kNullCheckpointId;
  std::vector<std::pair<std::string, std::uint64_t>> storageFields;
  std::vector<ValueSelector> selectors;
  std::vector<SavedSCC> sccs;
};

struct LoadedValueMapping {
  std::uint64_t value = kNullCheckpointId;
  std::uint64_t root = kNullCheckpointId;
};

struct LoadedOriginMapping {
  std::uint32_t variableId = 0;
  std::vector<std::uint64_t> values;
};

struct LoadedStore {
  std::uint64_t address = kNullCheckpointId;
  std::uint64_t source = kNullCheckpointId;
  std::uint32_t bitSize = 0;
};

struct LoadedPNDiffNode {
  std::uint64_t id = 0;
  std::string latticeType;
  bool conflict = false;
  std::vector<std::uint64_t> values;
};

struct LoadedPNDiffConstraint {
  bool isAdd = false;
  std::uint64_t sequence = 0;
  bool inWorklist = false;
  std::array<std::uint64_t, 3> values{};
  std::uint64_t instruction = kNullCheckpointId;
};

struct LoadedPNDiffState {
  std::uint64_t nextConstraintSequence = 0;
  std::vector<LoadedPNDiffNode> nodes;
  std::vector<LoadedPNDiffConstraint> constraints;
};

struct LoadedSCC {
  std::string name;
  std::uint32_t level = 0;
  std::vector<std::string> functions;
  std::vector<LoadedValueMapping> values;
  std::vector<std::uint64_t> contraValues;
  std::vector<LoadedOriginMapping> origins;
  std::vector<LoadedStore> stores;
  LoadedPNDiffState pndiff;
};

struct LoadedSemanticState {
  std::uint64_t rootCount = 0;
  std::uint64_t memoryRoot = kNullCheckpointId;
  std::uint64_t storageRoot = kNullCheckpointId;
  std::vector<std::pair<std::string, std::uint64_t>> storageFields;
  std::vector<ValueSelector> selectors;
  std::vector<LoadedSCC> sccs;
};

class BinaryWriter {
public:
  explicit BinaryWriter(llvm::StringRef path)
      : Label(path.str()), Output(Label, std::ios::binary | std::ios::trunc) {
    if (!Output) {
      failCheckpoint("cannot open " + path.str() + " for writing");
    }
  }

  void bytes(const char *data, std::size_t size) {
    Output.write(data, size);
    check();
  }

  template <typename UInt> void integer(UInt value) {
    static_assert(std::is_unsigned_v<UInt>);
    std::array<char, sizeof(UInt)> data{};
    for (std::size_t index = 0; index < data.size(); ++index) {
      data[index] = static_cast<char>(value & 0xffU);
      value >>= 8U;
    }
    bytes(data.data(), data.size());
  }

  void string(const std::string &value) {
    integer<std::uint64_t>(value.size());
    bytes(value.data(), value.size());
  }

  template <typename Range, typename WriteElement>
  void vector(const Range &range, WriteElement writeElement) {
    integer<std::uint64_t>(range.size());
    for (const auto &element : range) {
      writeElement(element);
    }
  }

  void finish() {
    Output.flush();
    check();
  }

private:
  void check() {
    if (!Output) {
      failCheckpoint("failed while writing " + Label);
    }
  }

  std::string Label;
  std::ofstream Output;
};

class BinaryReader {
public:
  explicit BinaryReader(llvm::StringRef path)
      : Label(path.str()), Input(Label, std::ios::binary) {
    if (!Input) {
      failCheckpoint("cannot open " + path.str() + " for reading");
    }
  }

  void bytes(char *data, std::size_t size, llvm::StringRef label) {
    Input.read(data, size);
    if (!Input) {
      failCheckpoint("truncated " + label.str() + " in " + Label);
    }
  }

  template <typename UInt> UInt integer(llvm::StringRef label) {
    static_assert(std::is_unsigned_v<UInt>);
    std::array<unsigned char, sizeof(UInt)> data{};
    bytes(reinterpret_cast<char *>(data.data()), data.size(), label);
    UInt value = 0;
    for (std::size_t index = 0; index < data.size(); ++index) {
      value |= static_cast<UInt>(data[index]) << (index * 8U);
    }
    return value;
  }

  std::uint64_t count(llvm::StringRef label) {
    auto value = integer<std::uint64_t>(label);
    if (value > kMaxCheckpointEntries ||
        value > static_cast<std::uint64_t>(
                    std::numeric_limits<std::size_t>::max())) {
      failCheckpoint(label.str() + " exceeds checkpoint limit");
    }
    return value;
  }

  std::string string(llvm::StringRef label) {
    auto size = integer<std::uint64_t>(label);
    if (size > kMaxCheckpointStringBytes ||
        size > static_cast<std::uint64_t>(
                   std::numeric_limits<std::size_t>::max())) {
      failCheckpoint(label.str() + " string exceeds checkpoint limit");
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    bytes(result.data(), result.size(), label);
    return result;
  }

  void finish() {
    if (Input.peek() != std::char_traits<char>::eof()) {
      failCheckpoint("trailing bytes in " + Label);
    }
  }

private:
  std::string Label;
  std::ifstream Input;
};

template <typename T, typename Key>
void sortBySelector(std::vector<T> &values, Key key) {
  std::sort(values.begin(), values.end(), [&](const T &left, const T &right) {
    return makeValueSelector(key(left)) < makeValueSelector(key(right));
  });
}

std::vector<std::string> stableFunctionList(const SCCData &data) {
  std::vector<std::string> result;
  result.reserve(data.SCCSet.size());
  for (auto *function : data.SCCSet) {
    result.push_back(toStableString(ExtValuePtr(function)));
  }
  std::sort(result.begin(), result.end());
  return result;
}

SavedPNDiffState collectPNDiffState(const ConstraintsGenerator &generator,
                                    ValueCatalog &values) {
  SavedPNDiffState saved;
  saved.nextConstraintSequence = generator.PG.NextConsSeq;

  for (const auto &[node, handles] : generator.PG.PNIMap.rev()) {
    if (node == nullptr || handles.empty()) {
      failCheckpoint("PNDiff contains an empty equivalence class");
    }
    SavedPNDiffNode savedNode;
    savedNode.id = node->getId();
    savedNode.latticeType = node->getLatticeTy().str();
    savedNode.conflict = node->isConflict();
    for (PNIValue handle : handles) {
      const auto &value = generator.getPNIExtValue(handle);
      savedNode.values.push_back(value);
      values.add(value);
    }
    sortBySelector(savedNode.values,
                   [](const ExtValuePtr &value) -> const ExtValuePtr & {
                     return value;
                   });
    saved.nodes.push_back(std::move(savedNode));
  }
  if (saved.nodes.size() != generator.PG.PNINodes.size()) {
    failCheckpoint("PNDiff contains an unmapped live node");
  }
  std::sort(saved.nodes.begin(), saved.nodes.end(),
            [](const SavedPNDiffNode &left, const SavedPNDiffNode &right) {
              return std::make_tuple(makeValueSelector(left.values.front()),
                                     left.id, left.latticeType, left.conflict) <
                     std::make_tuple(makeValueSelector(right.values.front()),
                                     right.id, right.latticeType,
                                     right.conflict);
            });

  for (const auto &constraint : generator.PG.Constraints) {
    SavedPNDiffConstraint savedConstraint;
    savedConstraint.isAdd = constraint.isAdd();
    savedConstraint.sequence = constraint.Seq;
    savedConstraint.inWorklist =
        generator.PG.Worklist.count(const_cast<ConsNode *>(&constraint)) != 0;
    auto handles = constraint.getNodes();
    for (std::size_t index = 0; index < handles.size(); ++index) {
      savedConstraint.values[index] = generator.getPNIExtValue(handles[index]);
      values.add(savedConstraint.values[index]);
    }
    if (constraint.getInst() != nullptr) {
      auto *instruction =
          static_cast<llvm::Instruction *>(constraint.getInst());
      savedConstraint.instruction = ExtValuePtr(instruction);
      values.add(*savedConstraint.instruction);
    }
    saved.constraints.push_back(std::move(savedConstraint));
  }
  std::sort(saved.constraints.begin(), saved.constraints.end(),
            [](const SavedPNDiffConstraint &left,
               const SavedPNDiffConstraint &right) {
              return left.sequence < right.sequence;
            });
  return saved;
}

SavedSemanticState collectSemanticState(const AllGraphs &graphs,
                                        SimpleType memoryType,
                                        SimpleType storageType,
                                        const std::map<std::string, SimpleType>
                                            &storageFields,
                                        RootCatalog &roots,
                                        ValueCatalog &values) {
  SavedSemanticState state;
  state.memoryRoot = roots.add(memoryType);
  state.storageRoot = roots.add(storageType);
  for (const auto &[name, type] : storageFields) {
    state.storageFields.emplace_back(name, roots.add(type));
  }

  state.sccs.reserve(graphs.AllSCCs.size());
  for (const auto &data : graphs.AllSCCs) {
    if (!data.Generator) {
      failCheckpoint("SCC has no completed ConstraintsGenerator: " +
                     data.SCCName);
    }
    const auto &generator = *data.Generator;
    if (!generator.unhandledCalls.empty()) {
      failCheckpoint("SCC still has unhandled calls: " + data.SCCName);
    }

    SavedSCC saved;
    saved.name = data.SCCName;
    saved.level = data.level;
    saved.functions = stableFunctionList(data);
    saved.values.reserve(generator.V2N.size());
    for (const auto &[value, type] : generator.V2N) {
      saved.values.push_back({value, roots.add(type)});
      values.add(value);
    }
    sortBySelector(saved.values,
                   [](const SavedValueMapping &mapping) -> const ExtValuePtr & {
                     return mapping.value;
                   });

    saved.contraValues.assign(generator.ContraVariantValues.begin(),
                              generator.ContraVariantValues.end());
    for (const auto &value : saved.contraValues) {
      values.add(value);
    }
    sortBySelector(saved.contraValues,
                   [](const ExtValuePtr &value) -> const ExtValuePtr & {
                     return value;
                   });

    for (const auto &[variableId, sourceValues] :
         generator.OriginalVariableSources) {
      SavedOriginMapping origin;
      origin.variableId = variableId;
      origin.values.assign(sourceValues.begin(), sourceValues.end());
      for (const auto &value : origin.values) {
        values.add(value);
      }
      sortBySelector(origin.values,
                     [](const ExtValuePtr &value) -> const ExtValuePtr & {
                       return value;
                     });
      saved.origins.push_back(std::move(origin));
    }

    for (const auto &[address, records] : generator.EVMStoreAccesses.StoresByAddr) {
      for (const auto &record : records) {
        if (record.Source == nullptr) {
          failCheckpoint("EVM store record has no source instruction");
        }
        SavedStore store{record.Addr, ExtValuePtr(record.Source), record.BitSize};
        saved.stores.push_back(store);
        values.add(store.address);
        values.add(store.source);
      }
    }
    std::sort(saved.stores.begin(), saved.stores.end(),
              [](const SavedStore &left, const SavedStore &right) {
                return std::make_tuple(makeValueSelector(left.address),
                                       makeValueSelector(left.source),
                                       left.bitSize) <
                       std::make_tuple(makeValueSelector(right.address),
                                       makeValueSelector(right.source),
                                       right.bitSize);
              });
    saved.pndiff = collectPNDiffState(generator, values);
    state.sccs.push_back(std::move(saved));
  }

  values.finalize();
  state.selectors = values.selectors();
  state.rootCount = roots.roots().size();
  return state;
}

void writeSemanticState(llvm::StringRef path, const SavedSemanticState &state,
                        const ValueCatalog &catalog) {
  BinaryWriter writer(path);
  writer.bytes(kValueMappingMagic.data(), kValueMappingMagic.size());
  writer.integer<std::uint32_t>(kValueMappingFormatVersion);
  writer.integer<std::uint64_t>(state.rootCount);
  writer.integer<std::uint64_t>(state.memoryRoot);
  writer.integer<std::uint64_t>(state.storageRoot);
  writer.vector(state.storageFields, [&](const auto &field) {
    writer.string(field.first);
    writer.integer<std::uint64_t>(field.second);
  });
  writer.vector(state.selectors, [&](const ValueSelector &selector) {
    writer.integer<std::uint8_t>(static_cast<std::uint8_t>(selector.kind));
    writer.string(selector.stable);
    writer.string(selector.type);
  });
  writer.vector(state.sccs, [&](const SavedSCC &scc) {
    writer.string(scc.name);
    writer.integer<std::uint32_t>(scc.level);
    writer.vector(scc.functions,
                  [&](const std::string &function) { writer.string(function); });
    writer.vector(scc.values, [&](const SavedValueMapping &mapping) {
      writer.integer<std::uint64_t>(catalog.id(mapping.value));
      writer.integer<std::uint64_t>(mapping.root);
    });
    writer.vector(scc.contraValues, [&](const ExtValuePtr &value) {
      writer.integer<std::uint64_t>(catalog.id(value));
    });
    writer.vector(scc.origins, [&](const SavedOriginMapping &origin) {
      writer.integer<std::uint32_t>(origin.variableId);
      writer.vector(origin.values, [&](const ExtValuePtr &value) {
        writer.integer<std::uint64_t>(catalog.id(value));
      });
    });
    writer.vector(scc.stores, [&](const SavedStore &store) {
      writer.integer<std::uint64_t>(catalog.id(store.address));
      writer.integer<std::uint64_t>(catalog.id(store.source));
      writer.integer<std::uint32_t>(store.bitSize);
    });
  });
  writer.finish();
}

void writePNDiffState(llvm::StringRef path, const SavedSemanticState &state,
                      const ValueCatalog &catalog) {
  BinaryWriter writer(path);
  writer.bytes(kPNDiffStateMagic.data(), kPNDiffStateMagic.size());
  writer.integer<std::uint32_t>(kPNDiffStateFormatVersion);
  writer.vector(state.sccs, [&](const SavedSCC &scc) {
    writer.integer<std::uint64_t>(scc.pndiff.nextConstraintSequence);
    writer.vector(scc.pndiff.nodes, [&](const SavedPNDiffNode &node) {
      writer.integer<std::uint64_t>(node.id);
      writer.string(node.latticeType);
      writer.integer<std::uint8_t>(node.conflict);
      writer.vector(node.values, [&](const ExtValuePtr &value) {
        writer.integer<std::uint64_t>(catalog.id(value));
      });
    });
    writer.vector(scc.pndiff.constraints,
                  [&](const SavedPNDiffConstraint &constraint) {
      writer.integer<std::uint8_t>(constraint.isAdd);
      writer.integer<std::uint64_t>(constraint.sequence);
      writer.integer<std::uint8_t>(constraint.inWorklist);
      for (const auto &value : constraint.values) {
        writer.integer<std::uint64_t>(catalog.id(value));
      }
      writer.integer<std::uint64_t>(
          constraint.instruction ? catalog.id(*constraint.instruction)
                                 : kNullCheckpointId);
    });
  });
  writer.finish();
}

LoadedSemanticState readSemanticState(llvm::StringRef path) {
  BinaryReader reader(path);
  std::array<char, kValueMappingMagic.size()> magic{};
  reader.bytes(magic.data(), magic.size(), "mapping magic");
  if (magic != kValueMappingMagic) {
    failCheckpoint("invalid value-mapping.bin magic");
  }
  if (reader.integer<std::uint32_t>("mapping version") !=
      kValueMappingFormatVersion) {
    failCheckpoint("unsupported value mapping format version");
  }

  LoadedSemanticState state;
  state.rootCount = reader.integer<std::uint64_t>("root count");
  state.memoryRoot = reader.integer<std::uint64_t>("memory root");
  state.storageRoot = reader.integer<std::uint64_t>("storage root");
  auto fieldCount = reader.count("storage field count");
  for (std::uint64_t index = 0; index < fieldCount; ++index) {
    // Keep stream reads sequenced explicitly. Function argument evaluation
    // order is not guaranteed and swapping these reads corrupts the format.
    auto name = reader.string("storage field name");
    auto root = reader.integer<std::uint64_t>("storage field root");
    state.storageFields.emplace_back(std::move(name), root);
  }

  auto selectorCount = reader.count("value selector count");
  state.selectors.reserve(selectorCount);
  std::set<ValueSelector> uniqueSelectors;
  for (std::uint64_t index = 0; index < selectorCount; ++index) {
    ValueSelector selector;
    selector.kind = static_cast<ValueSelectorKind>(
        reader.integer<std::uint8_t>("value selector kind"));
    if (selector.kind < ValueSelectorKind::LLVMValue ||
        selector.kind > ValueSelectorKind::HeapObject) {
      failCheckpoint("unknown value selector kind");
    }
    selector.stable = reader.string("value selector stable ID");
    selector.type = reader.string("value selector type");
    if (!uniqueSelectors.insert(selector).second) {
      failCheckpoint("duplicate value selector in mapping file");
    }
    state.selectors.push_back(std::move(selector));
  }

  auto sccCount = reader.count("SCC count");
  state.sccs.reserve(sccCount);
  for (std::uint64_t sccIndex = 0; sccIndex < sccCount; ++sccIndex) {
    LoadedSCC scc;
    scc.name = reader.string("SCC name");
    scc.level = reader.integer<std::uint32_t>("SCC level");
    auto functionCount = reader.count("SCC function count");
    for (std::uint64_t index = 0; index < functionCount; ++index) {
      scc.functions.push_back(reader.string("SCC function"));
    }
    auto valueCount = reader.count("V2N count");
    for (std::uint64_t index = 0; index < valueCount; ++index) {
      scc.values.push_back(
          {reader.integer<std::uint64_t>("V2N value"),
           reader.integer<std::uint64_t>("V2N root")});
    }
    auto contraCount = reader.count("contra value count");
    for (std::uint64_t index = 0; index < contraCount; ++index) {
      scc.contraValues.push_back(
          reader.integer<std::uint64_t>("contra value"));
    }
    auto originCount = reader.count("origin count");
    for (std::uint64_t index = 0; index < originCount; ++index) {
      LoadedOriginMapping origin;
      origin.variableId = reader.integer<std::uint32_t>("origin variable ID");
      auto sourceCount = reader.count("origin source count");
      for (std::uint64_t source = 0; source < sourceCount; ++source) {
        origin.values.push_back(
            reader.integer<std::uint64_t>("origin value"));
      }
      scc.origins.push_back(std::move(origin));
    }
    auto storeCount = reader.count("EVM store count");
    for (std::uint64_t index = 0; index < storeCount; ++index) {
      scc.stores.push_back(
          {reader.integer<std::uint64_t>("EVM store address"),
           reader.integer<std::uint64_t>("EVM store source"),
           reader.integer<std::uint32_t>("EVM store bit size")});
    }
    state.sccs.push_back(std::move(scc));
  }
  reader.finish();
  return state;
}

void readPNDiffState(llvm::StringRef path, LoadedSemanticState &state) {
  BinaryReader reader(path);
  std::array<char, kPNDiffStateMagic.size()> magic{};
  reader.bytes(magic.data(), magic.size(), "PNDiff magic");
  if (magic != kPNDiffStateMagic) {
    failCheckpoint("invalid pndiff-state.bin magic");
  }
  if (reader.integer<std::uint32_t>("PNDiff version") !=
      kPNDiffStateFormatVersion) {
    failCheckpoint("unsupported PNDiff state format version");
  }
  auto sccCount = reader.count("PNDiff SCC count");
  if (sccCount != state.sccs.size()) {
    failCheckpoint("PNDiff/value mapping SCC count mismatch");
  }
  for (auto &scc : state.sccs) {
    scc.pndiff.nextConstraintSequence =
        reader.integer<std::uint64_t>("PNDiff next constraint sequence");
    auto nodeCount = reader.count("PNDiff node count");
    for (std::uint64_t index = 0; index < nodeCount; ++index) {
      LoadedPNDiffNode node;
      node.id = reader.integer<std::uint64_t>("PNDiff node ID");
      node.latticeType = reader.string("PNDiff lattice type");
      auto conflict = reader.integer<std::uint8_t>("PNDiff conflict flag");
      if (conflict > 1) {
        failCheckpoint("invalid PNDiff conflict flag");
      }
      node.conflict = conflict != 0;
      auto valueCount = reader.count("PNDiff node value count");
      if (valueCount == 0) {
        failCheckpoint("PNDiff node has no mapped values");
      }
      for (std::uint64_t value = 0; value < valueCount; ++value) {
        node.values.push_back(
            reader.integer<std::uint64_t>("PNDiff node value"));
      }
      scc.pndiff.nodes.push_back(std::move(node));
    }
    auto constraintCount = reader.count("PNDiff constraint count");
    for (std::uint64_t index = 0; index < constraintCount; ++index) {
      LoadedPNDiffConstraint constraint;
      auto isAdd = reader.integer<std::uint8_t>("PNDiff constraint kind");
      if (isAdd > 1) {
        failCheckpoint("invalid PNDiff constraint kind");
      }
      constraint.isAdd = isAdd != 0;
      constraint.sequence =
          reader.integer<std::uint64_t>("PNDiff constraint sequence");
      auto inWorklist =
          reader.integer<std::uint8_t>("PNDiff worklist flag");
      if (inWorklist > 1) {
        failCheckpoint("invalid PNDiff worklist flag");
      }
      constraint.inWorklist = inWorklist != 0;
      for (auto &value : constraint.values) {
        value = reader.integer<std::uint64_t>("PNDiff constraint value");
      }
      constraint.instruction =
          reader.integer<std::uint64_t>("PNDiff constraint instruction");
      scc.pndiff.constraints.push_back(constraint);
    }
  }
  reader.finish();
}

void addReturnCandidates(const llvm::Function &function,
                         const std::function<void(const ExtValuePtr &)> &add) {
  llvm::Type *type = function.getReturnType();
  if (type->isVoidTy()) {
    return;
  }
  auto *mutableFunction = const_cast<llvm::Function *>(&function);
  if (auto *structure = llvm::dyn_cast<llvm::StructType>(type)) {
    for (unsigned index = 0; index < structure->getNumElements(); ++index) {
      add(ReturnValue{mutableFunction, static_cast<std::int32_t>(index)});
    }
    return;
  }
  if (auto *array = llvm::dyn_cast<llvm::ArrayType>(type)) {
    for (std::uint64_t index = 0; index < array->getNumElements(); ++index) {
      if (index > static_cast<std::uint64_t>(
                      std::numeric_limits<std::int32_t>::max())) {
        failCheckpoint("aggregate return index exceeds checkpoint range");
      }
      add(ReturnValue{mutableFunction, static_cast<std::int32_t>(index)});
    }
    return;
  }
  add(ReturnValue{mutableFunction});
}

std::vector<ExtValuePtr>
resolveValueSelectors(const llvm::Module &module,
                      const std::vector<ValueSelector> &selectors) {
  struct Candidate {
    std::optional<ExtValuePtr> value;
    bool collision = false;
  };
  std::map<ValueSelector, Candidate> candidates;
  for (const auto &selector : selectors) {
    candidates.emplace(selector, Candidate{});
  }

  auto add = [&](const ExtValuePtr &value) {
    auto selector = makeValueSelector(value);
    auto found = candidates.find(selector);
    if (found == candidates.end()) {
      return;
    }
    if (!found->second.value) {
      found->second.value = value;
    } else if (!sameExtValue(*found->second.value, value)) {
      found->second.collision = true;
    }
  };

  // UConstant selectors include their immediate use site. Some of those users
  // are ConstantExpr/aggregate constants rather than instructions, so walk the
  // reachable constant DAG as well as instruction operands. Constants are
  // uniqued and shared by LLVM; the visited set prevents repeated expansion.
  llvm::DenseSet<const llvm::User *> expandedUsers;
  std::function<void(llvm::User &)> addOperands = [&](llvm::User &user) {
    if (!expandedUsers.insert(&user).second) {
      return;
    }
    for (unsigned index = 0; index < user.getNumOperands(); ++index) {
      auto *operand = user.getOperand(index);
      add(getExtValuePtr(operand, &user, index));
      if (auto *constant = llvm::dyn_cast<llvm::ConstantInt>(operand)) {
        add(ConstantAddr{constant});
      }
      if (auto *nested = llvm::dyn_cast<llvm::Constant>(operand);
          nested != nullptr && nested->getNumOperands() != 0) {
        addOperands(*nested);
      }
    }
  };

  for (const llvm::GlobalValue &global : module.global_values()) {
    add(ExtValuePtr(const_cast<llvm::GlobalValue *>(&global)));
  }
  for (const llvm::Function &function : module) {
    addReturnCandidates(function, add);
    for (const llvm::Argument &argument : function.args()) {
      add(ExtValuePtr(const_cast<llvm::Argument *>(&argument)));
    }
    for (const llvm::BasicBlock &block : function) {
      add(ExtValuePtr(const_cast<llvm::BasicBlock *>(&block)));
      for (const llvm::Instruction &instruction : block) {
        auto *mutableInstruction = const_cast<llvm::Instruction *>(&instruction);
        add(ExtValuePtr(mutableInstruction));
        if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(mutableInstruction)) {
          add(StackObject{alloca});
        }
        if (auto *call = llvm::dyn_cast<llvm::CallBase>(mutableInstruction)) {
          add(HeapObject{call});
        }
        // ConstantAddr and UConstant deliberately identify different semantic
        // roles for the same LLVM ConstantInt; addOperands reconstructs both.
        addOperands(*mutableInstruction);
      }
    }
  }

  // Global initializer DAGs can be large and are not normally part of V2N.
  // Traverse them only when the instruction scan left a constant selector
  // unresolved.
  const bool needsGlobalConstantScan = std::any_of(
      candidates.begin(), candidates.end(), [](const auto &entry) {
        return !entry.second.value &&
               (entry.first.kind == ValueSelectorKind::UConstant ||
                entry.first.kind == ValueSelectorKind::ConstantAddr);
      });
  if (needsGlobalConstantScan) {
    for (const llvm::GlobalValue &global : module.global_values()) {
      auto *mutableGlobal = const_cast<llvm::GlobalValue *>(&global);
      if (auto *user = llvm::dyn_cast<llvm::User>(mutableGlobal)) {
        addOperands(*user);
      }
    }
  }

  std::vector<ExtValuePtr> resolved;
  resolved.reserve(selectors.size());
  for (const auto &selector : selectors) {
    const auto &candidate = candidates.at(selector);
    if (!candidate.value) {
      failCheckpoint("value selector not found in current IR: " +
                     selector.stable);
    }
    if (candidate.collision) {
      failCheckpoint("value selector is ambiguous in current IR: " +
                     selector.stable);
    }
    resolved.push_back(*candidate.value);
  }
  return resolved;
}

std::string checkpointConfigurationDigest(const MLsubRecovery &recovery) {
  std::ostringstream text;
  text << "binarysub_state_abi=" << kBinarysubStateABI << '\n';
  text << "pointer_size=" << recovery.getCheckpointPointerSize() << '\n';
  text << "merge_struct_ptr_load_store="
       << recovery.getCheckpointStructMergeEnabled() << '\n';
  text << "default_poly=" << environmentValue("NOTDEC_DEFAULT_POLY") << '\n';
  text << "local_subtype_merge="
       << environmentValue("NOTDEC_LOCAL_SUBTYPE_MERGE_MODE") << '\n';
  text << "early_call_interface_merge="
       << environmentValue("NOTDEC_EARLY_CALL_INTERFACE_MERGE") << '\n';
  text << "poly_file=" << hashOptionalFileEnvironment("NOTDEC_POLY_FUNCS")
       << '\n';
  text << "pndiff_policy="
       << hashOptionalFileEnvironment("NOTDEC_PNDIFF_POLICY_OVERRIDE") << '\n';
  text << "summary_doc="
       << llvm::formatv("{0}", recovery.getCheckpointSummaryDoc()).str() << '\n';
  text << "signature_doc="
       << llvm::formatv("{0}", recovery.getCheckpointSignatureDoc()).str()
       << '\n';
  text << "extra_doc="
       << llvm::formatv("{0}", recovery.getCheckpointExtraConstraintsDoc()).str()
       << '\n';
  std::vector<std::string> mallocWrappers;
  for (const auto *function : recovery.getCheckpointMallocWrappers()) {
    mallocWrappers.push_back(function->getName().str());
  }
  std::sort(mallocWrappers.begin(), mallocWrappers.end());
  for (const auto &name : mallocWrappers) {
    text << "malloc_wrapper=" << name << '\n';
  }
  std::vector<std::string> polymorphicBuffers;
  for (const auto *function :
       recovery.getCheckpointPolymorphicBufferFunctions()) {
    polymorphicBuffers.push_back(function->getName().str());
  }
  std::sort(polymorphicBuffers.begin(), polymorphicBuffers.end());
  for (const auto &name : polymorphicBuffers) {
    text << "poly_buffer=" << name << '\n';
  }
  return sha256(text.str());
}

llvm::json::Object readManifest(llvm::StringRef directory) {
  auto content = readFile(checkpointPath(directory, kManifestFile));
  auto parsed = llvm::json::parse(content);
  if (!parsed) {
    failCheckpoint("manifest.json is not valid JSON");
  }
  auto *object = parsed->getAsObject();
  if (object == nullptr) {
    failCheckpoint("manifest.json must contain an object");
  }
  return std::move(*object);
}

std::int64_t manifestInteger(const llvm::json::Object &manifest,
                             llvm::StringRef key) {
  auto value = manifest.getInteger(key);
  if (!value) {
    failCheckpoint("manifest field '" + key.str() + "' is missing or invalid");
  }
  return *value;
}

std::string manifestString(const llvm::json::Object &manifest,
                           llvm::StringRef key) {
  auto value = manifest.getString(key);
  if (!value) {
    failCheckpoint("manifest field '" + key.str() + "' is missing or invalid");
  }
  return value->str();
}

void validateManifest(const llvm::json::Object &manifest,
                      const MLsubRecovery &recovery,
                      llvm::StringRef moduleSHA256Hex) {
  if (manifestInteger(manifest, "format_version") !=
      kCheckpointFormatVersion) {
    failCheckpoint("unsupported checkpoint format version");
  }
  if (manifestString(manifest, "stage") !=
      "post-constraint/pre-simplify") {
    failCheckpoint("checkpoint stage is not post-constraint/pre-simplify");
  }
  if (manifestString(manifest, "ir_sha256") != moduleSHA256Hex) {
    failCheckpoint("frozen IR SHA-256 mismatch");
  }
  if (manifestString(manifest, "data_layout") !=
      recovery.getCheckpointDataLayout()) {
    failCheckpoint("LLVM data layout mismatch");
  }
  if (manifestString(manifest, "target_triple") !=
      recovery.getCheckpointModule().getTargetTriple().str()) {
    failCheckpoint("LLVM target triple mismatch");
  }
  if (manifestInteger(manifest, "pointer_size_bits") !=
      recovery.getCheckpointPointerSize()) {
    failCheckpoint("pointer size mismatch");
  }
  if (manifestInteger(manifest, "tr_level") !=
      recovery.getCheckpointTypeRecoveryLevel()) {
    failCheckpoint("type-recovery level mismatch");
  }
  if (manifestInteger(manifest, "binarysub_state_abi") !=
      kBinarysubStateABI) {
    failCheckpoint("binarysub state ABI mismatch");
  }
  if (manifestString(manifest, "configuration_sha256") !=
      checkpointConfigurationDigest(recovery)) {
    failCheckpoint("constraint-generation configuration mismatch");
  }
  auto pndiff = manifest.getBoolean("pndiff_state_saved");
  if (!pndiff || !*pndiff) {
    failCheckpoint("unsupported PNDiff checkpoint capability");
  }
}

llvm::json::Value buildManifest(
    const MLsubRecovery &recovery, llvm::StringRef moduleSHA256Hex,
    const binarysub::SimpleTypeGraphSnapshot &graph,
    const SavedSemanticState &semantic) {
  std::uint64_t lowerEdges = 0;
  std::uint64_t upperEdges = 0;
  std::uint64_t structuralEdges = 0;
  for (const auto &node : graph.nodes) {
    if (node.kind == binarysub::SimpleTypeSnapshotNodeKind::Variable) {
      lowerEdges += node.lowerBounds.size();
      upperEdges += node.upperBounds.size();
    } else if (node.kind == binarysub::SimpleTypeSnapshotNodeKind::Function) {
      structuralEdges += node.functionArgs.size() +
                         (node.functionResult !=
                          binarysub::kNullSimpleTypeSnapshotNodeId);
    } else if (node.kind == binarysub::SimpleTypeSnapshotNodeKind::MemObject) {
      structuralEdges += node.fields.size() + node.directLoad.has_value() +
                         node.directStore.has_value();
    }
  }

  llvm::json::Object manifest;
  manifest["format_version"] = static_cast<std::int64_t>(kCheckpointFormatVersion);
  manifest["stage"] = "post-constraint/pre-simplify";
  manifest["ir_sha256"] = moduleSHA256Hex.str();
  manifest["data_layout"] = recovery.getCheckpointDataLayout().str();
  manifest["target_triple"] =
      recovery.getCheckpointModule().getTargetTriple().str();
  manifest["pointer_size_bits"] =
      static_cast<std::int64_t>(recovery.getCheckpointPointerSize());
  manifest["tr_level"] =
      static_cast<std::int64_t>(recovery.getCheckpointTypeRecoveryLevel());
  manifest["binarysub_state_abi"] =
      static_cast<std::int64_t>(kBinarysubStateABI);
  manifest["binarysub_snapshot_version"] =
      static_cast<std::int64_t>(graph.formatVersion);
  manifest["value_mapping_version"] =
      static_cast<std::int64_t>(kValueMappingFormatVersion);
  manifest["pndiff_state_version"] =
      static_cast<std::int64_t>(kPNDiffStateFormatVersion);
  manifest["configuration_sha256"] =
      checkpointConfigurationDigest(recovery);
  manifest["scc_count"] = static_cast<std::int64_t>(semantic.sccs.size());
  manifest["graph_nodes"] = static_cast<std::int64_t>(graph.nodes.size());
  manifest["graph_roots"] = static_cast<std::int64_t>(graph.roots.size());
  manifest["lower_bound_edges"] = static_cast<std::int64_t>(lowerEdges);
  manifest["upper_bound_edges"] = static_cast<std::int64_t>(upperEdges);
  manifest["structural_edges"] = static_cast<std::int64_t>(structuralEdges);
  manifest["value_count"] = static_cast<std::int64_t>(semantic.selectors.size());
  manifest["pndiff_state_saved"] = true;
  manifest["merge_eval_history_saved"] = false;
  manifest["files"] = llvm::json::Array{
      kInputIRFile.str(), kInputBitcodeFile.str(), kSimpleTypeGraphFile.str(),
      kValueMappingFile.str(), kPNDiffStateFile.str()};
  return llvm::json::Value(std::move(manifest));
}

SimpleType rootAt(const std::vector<SimpleType> &roots, std::uint64_t id,
                  bool allowNull, llvm::StringRef label) {
  if (id == kNullCheckpointId && allowNull) {
    return nullptr;
  }
  if (id >= roots.size()) {
    failCheckpoint(label.str() + " root ID is out of range");
  }
  if (!roots[id]) {
    failCheckpoint(label.str() + " unexpectedly resolves to null");
  }
  return roots[id];
}

const ExtValuePtr &valueAt(const std::vector<ExtValuePtr> &values,
                           std::uint64_t id, llvm::StringRef label) {
  if (id >= values.size()) {
    failCheckpoint(label.str() + " ValueId is out of range");
  }
  return values[id];
}

void restorePNDiffState(ConstraintsGenerator &generator,
                        const LoadedPNDiffState &saved,
                        const std::vector<ExtValuePtr> &values) {
  std::set<PNIValue> mappedHandles;
  std::set<std::uint64_t> nodeIds;
  for (const auto &savedNode : saved.nodes) {
    if (savedNode.id > std::numeric_limits<unsigned long>::max() ||
        !nodeIds.insert(savedNode.id).second) {
      failCheckpoint("invalid or duplicate PNDiff node ID");
    }
    PNINode *node = nullptr;
    try {
      node = generator.PG.restorePNINode(
          savedNode.latticeType, static_cast<unsigned long>(savedNode.id));
    } catch (const std::exception &error) {
      failCheckpoint("invalid PNDiff lattice type '" + savedNode.latticeType +
                     "': " + error.what());
    }
    if (node == nullptr || node->getLatticeTy().str() != savedNode.latticeType) {
      failCheckpoint("PNDiff lattice type did not round-trip");
    }
    if (savedNode.conflict) {
      node->setConflict();
    }
    for (auto valueId : savedNode.values) {
      const auto &value = valueAt(values, valueId, "PNDiff node");
      PNIValue handle = generator.getPNIValue(value);
      if (!mappedHandles.insert(handle).second ||
          !generator.PG.PNIMap.insert(handle, node).second) {
        failCheckpoint("PNDiff value is mapped to multiple nodes");
      }
    }
  }

  std::uint64_t maximumSequence = 0;
  bool hasConstraint = false;
  for (const auto &savedConstraint : saved.constraints) {
    std::array<PNIValue, 3> handles{};
    for (std::size_t index = 0; index < handles.size(); ++index) {
      const auto &value =
          valueAt(values, savedConstraint.values[index], "PNDiff constraint");
      handles[index] = generator.getPNIValue(value);
      if (generator.PG.getPNIVarOrNull(handles[index]) == nullptr) {
        failCheckpoint("PNDiff constraint references an unmapped node");
      }
    }
    llvm::Instruction *instruction = nullptr;
    if (savedConstraint.instruction != kNullCheckpointId) {
      const auto &instructionValue = valueAt(
          values, savedConstraint.instruction, "PNDiff instruction");
      auto pointer = std::get_if<llvm::Value *>(&instructionValue);
      instruction = pointer == nullptr
                        ? nullptr
                        : llvm::dyn_cast_or_null<llvm::Instruction>(*pointer);
      if (instruction == nullptr) {
        failCheckpoint("PNDiff instruction selector is not an instruction");
      }
    }
    if (savedConstraint.isAdd) {
      generator.PG.addAddCons(handles[0], handles[1], handles[2], instruction);
    } else {
      generator.PG.addSubCons(handles[0], handles[1], handles[2], instruction);
    }
    auto &constraint = generator.PG.Constraints.back();
    generator.PG.Worklist.erase(&constraint);
    constraint.Seq = savedConstraint.sequence;
    if (savedConstraint.inWorklist) {
      if (!generator.PG.Worklist.insert(&constraint).second) {
        failCheckpoint("duplicate PNDiff worklist sequence");
      }
    }
    maximumSequence = std::max(maximumSequence, savedConstraint.sequence);
    hasConstraint = true;
  }
  if (hasConstraint && saved.nextConstraintSequence <= maximumSequence) {
    failCheckpoint("PNDiff next constraint sequence overlaps saved state");
  }
  generator.PG.NextConsSeq = saved.nextConstraintSequence;
}

} // namespace

void MLsubRecovery::savePostConstraintState(
    llvm::StringRef directory, llvm::StringRef moduleSHA256Hex) {
  if (directory.empty()) {
    return;
  }
  const auto timing = startCheckpointTiming();
  if (llvm::sys::fs::exists(directory)) {
    llvm::errs() << "Error: checkpoint target already exists: " << directory
                 << "\n";
    std::abort();
  }

  llvm::SmallString<256> parent(llvm::sys::path::parent_path(directory));
  if (!parent.empty()) {
    if (auto error = llvm::sys::fs::create_directories(parent)) {
      llvm::errs() << "Error: cannot create checkpoint parent directory "
                   << parent << ": " << error.message() << "\n";
      std::abort();
    }
  }

  llvm::SmallString<256> temporary;
  if (auto error = llvm::sys::fs::createUniqueDirectory(
          directory.str() + ".tmp", temporary)) {
    llvm::errs() << "Error: cannot create checkpoint temporary directory: "
                 << error.message() << "\n";
    std::abort();
  }

  try {
    RootCatalog roots;
    ValueCatalog values;
    auto semantic = collectSemanticState(AG, MemoryType, StorageType,
                                         StorageFields, roots, values);
    auto graphResult = binarysub::createSimpleTypeGraphSnapshot(
        binarysub::globalTypeContext(), roots.roots());
    if (!graphResult) {
      failCheckpoint(graphResult.error().msg);
    }
    auto graph = std::move(graphResult.value());

    writeTextFile(checkpointPath(temporary, kInputIRFile), renderModule(Mod));
    writeBitcodeFile(Mod, checkpointPath(temporary, kInputBitcodeFile));
    {
      std::ofstream output(checkpointPath(temporary, kSimpleTypeGraphFile),
                           std::ios::binary | std::ios::trunc);
      if (!output) {
        failCheckpoint("cannot open simple-type-graph.bin for writing");
      }
      auto result = binarysub::writeSimpleTypeGraphSnapshot(output, graph);
      if (!result) {
        failCheckpoint(result.error().msg);
      }
    }
    writeSemanticState(checkpointPath(temporary, kValueMappingFile), semantic,
                       values);
    writePNDiffState(checkpointPath(temporary, kPNDiffStateFile), semantic,
                     values);
    writeTextFile(checkpointPath(temporary, kManifestFile),
                  llvm::formatv("{0:2}\n", buildManifest(
                                                   *this, moduleSHA256Hex,
                                                   graph, semantic))
                      .str());

    if (auto error = llvm::sys::fs::rename(temporary, directory)) {
      failCheckpoint("cannot publish checkpoint directory: " +
                     error.message());
    }
    llvm::errs() << "[post-constraint-checkpoint] action=save directory="
                 << directory << " ir_sha256=" << moduleSHA256Hex
                 << " sccs=" << semantic.sccs.size()
                 << " graph_nodes=" << graph.nodes.size()
                 << " graph_roots=" << graph.roots.size()
                 << " values=" << semantic.selectors.size();
    printCheckpointTiming(timing);
    llvm::errs() << "\n";
  } catch (const CheckpointFailure &failure) {
    llvm::sys::fs::remove_directories(temporary);
    llvm::errs() << "Error: cannot save post-constraint checkpoint: "
                 << failure.what() << "\n";
    std::abort();
  }
}

void MLsubRecovery::loadPostConstraintState(
    llvm::StringRef directory, llvm::StringRef moduleSHA256Hex) {
  if (directory.empty()) {
    return;
  }
  const auto timing = startCheckpointTiming();
  try {
    if (!MergeEvalDir.empty()) {
      failCheckpoint("MergeEval history is not available in format version 1");
    }
    auto manifest = readManifest(directory);
    validateManifest(manifest, *this, moduleSHA256Hex);
    auto semantic =
        readSemanticState(checkpointPath(directory, kValueMappingFile));
    readPNDiffState(checkpointPath(directory, kPNDiffStateFile), semantic);
    if (semantic.sccs.size() != AG.AllSCCs.size()) {
      failCheckpoint("SCC count mismatch");
    }
    if (semantic.rootCount !=
        static_cast<std::uint64_t>(manifestInteger(manifest, "graph_roots"))) {
      failCheckpoint("mapping/manifest root count mismatch");
    }

    std::ifstream graphInput(checkpointPath(directory, kSimpleTypeGraphFile),
                             std::ios::binary);
    if (!graphInput) {
      failCheckpoint("cannot open simple-type-graph.bin for reading");
    }
    auto graphResult = binarysub::readSimpleTypeGraphSnapshot(graphInput);
    if (!graphResult) {
      failCheckpoint(graphResult.error().msg);
    }
    auto graph = std::move(graphResult.value());
    if (graph.roots.size() != semantic.rootCount ||
        graph.nodes.size() !=
            static_cast<std::uint64_t>(
                manifestInteger(manifest, "graph_nodes"))) {
      failCheckpoint("graph statistics do not match manifest/mapping");
    }

    auto restoredResult = binarysub::restoreSimpleTypeGraphSnapshot(
        binarysub::globalTypeContext(), graph);
    if (!restoredResult) {
      failCheckpoint(restoredResult.error().msg);
    }
    auto roots = std::move(restoredResult.value());
    auto resolvedValues = resolveValueSelectors(Mod, semantic.selectors);

    MemoryType = rootAt(roots, semantic.memoryRoot, false, "memory");
    StorageType = rootAt(roots, semantic.storageRoot, true, "storage");
    StorageFields.clear();
    for (const auto &[name, root] : semantic.storageFields) {
      if (!StorageFields.emplace(name, rootAt(roots, root, false,
                                              "storage field"))
               .second) {
        failCheckpoint("duplicate storage field name");
      }
    }

    for (std::size_t index = 0; index < AG.AllSCCs.size(); ++index) {
      auto &data = AG.AllSCCs[index];
      const auto &saved = semantic.sccs[index];
      if (data.SCCName != saved.name || data.level != saved.level ||
          stableFunctionList(data) != saved.functions) {
        failCheckpoint("SCC identity mismatch at index " +
                       std::to_string(index));
      }

      std::set<llvm::Function *> opaqueBodies;
      for (auto *function : data.SCCSet) {
        if (isOpaqueBody(*function)) {
          opaqueBodies.insert(function);
        }
      }
      data.Generator = std::make_shared<ConstraintsGenerator>(
          data.SCCName, PointerSize, data.SCCSet, MemoryType, StorageType,
          &StorageFields, data.level, BinarysubTraceFile.get(), nullptr,
          EnableStructPtrLoadStoreMerge, std::move(opaqueBodies));
      auto &generator = *data.Generator;

      for (const auto &mapping : saved.values) {
        const auto &value = valueAt(resolvedValues, mapping.value, "V2N");
        auto type = rootAt(roots, mapping.root, false, "V2N");
        if (!generator.V2N.insert(value, type).second) {
          failCheckpoint("duplicate V2N value in SCC " + data.SCCName);
        }
        generator.attachExternalValueHandle(type, value);
      }
      for (auto valueId : saved.contraValues) {
        generator.ContraVariantValues.insert(
            valueAt(resolvedValues, valueId, "contra value"));
      }
      for (const auto &origin : saved.origins) {
        auto &target = generator.OriginalVariableSources[origin.variableId];
        for (auto valueId : origin.values) {
          target.insert(valueAt(resolvedValues, valueId, "origin value"));
        }
      }
      for (const auto &store : saved.stores) {
        const auto &address =
            valueAt(resolvedValues, store.address, "EVM store address");
        const auto &sourceValue =
            valueAt(resolvedValues, store.source, "EVM store source");
        auto pointer = std::get_if<llvm::Value *>(&sourceValue);
        auto *source = pointer == nullptr
                           ? nullptr
                           : llvm::dyn_cast_or_null<llvm::Instruction>(*pointer);
        if (source == nullptr) {
          failCheckpoint("EVM store source is not an instruction");
        }
        generator.EVMStoreAccesses.StoresByAddr[address].push_back(RecordedStore{
            .Addr = address,
            .ValueTy = nullptr,
            .BitSize = store.bitSize,
            .Source = source,
        });
      }
      restorePNDiffState(generator, saved.pndiff, resolvedValues);
    }

    LoadedPostConstraintState = true;
    llvm::errs() << "[post-constraint-checkpoint] action=load directory="
                 << directory << " ir_sha256=" << moduleSHA256Hex
                 << " sccs=" << semantic.sccs.size()
                 << " graph_nodes=" << graph.nodes.size()
                 << " graph_roots=" << graph.roots.size()
                 << " values=" << semantic.selectors.size()
                 << " bottom_up_skipped=true top_down_skipped=true"
                    " pndiff_state_restored=true";
    printCheckpointTiming(timing);
    llvm::errs() << "\n";
  } catch (const CheckpointFailure &failure) {
    llvm::errs() << "Error: cannot load post-constraint checkpoint: "
                 << failure.what() << "\n";
    std::abort();
  }
}

} // namespace notdec::mlsub
