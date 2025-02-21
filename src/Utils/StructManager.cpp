#include "Utils/StructManager.h"
#include "TypeRecovery/Parser.h"
#include "optimizers/ConstraintGenerator.h"
#include <cassert>
#include <cstddef>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/JSON.h>
#include <memory>
#include <string>
#include <vector>

namespace notdec {

void StructInfo::addPaddings() {
  for (size_t i = 0; i < Fields.size(); i++) {
    auto &Ent = Fields[i];
    if (i + 1 < Fields.size()) {
      auto &Next = Fields[i + 1];
      auto NextOffset = Next.R.Start.offset;
      auto CurEnd = Ent.R.Start.offset + Ent.R.Size;
      if (CurEnd < NextOffset) {
        if (Ent.R.Start.access.size() > 0) {
          // expand the array size
          Ent.R.Size = NextOffset - Ent.R.Start.offset;
        } else {
          // add padding
          Fields.insert(
              Fields.begin() + i + 1,
              FieldEntry{.R = Range{.Start = IndexTy{.offset = CurEnd},
                                    .Size = NextOffset - CurEnd},
                         .isPadding = true});
        }
      }
    }
  }
}

std::shared_ptr<BytesManager> BytesManager::create(llvm::Module &M) {
  std::shared_ptr<BytesManager> BM = std::make_shared<BytesManager>();
  for (auto &G : M.getGlobalList()) {
    auto Name = G.getName();
    if (!Name.consume_front("__notdec_mem0_0x")) {
      continue;
    }
    auto Offset = std::stol(Name.str(), nullptr, 16);
    assert(Offset > 0);
    if (G.hasInitializer()) {
      auto *Init = G.getInitializer();
      if (auto *C = llvm::dyn_cast<llvm::ConstantDataArray>(Init)) {
        auto Data = C->getRawDataValues();
        BM->Bytes.push_back({Range{.Start = IndexTy{.offset = Offset},
                                   .Size = static_cast<OffsetTy>(Data.size())},
                             Data});
      }
    }
  }
  return BM;
}

llvm::StringRef decodeCStr1(llvm::StringRef Bytes, int64_t Offset) {
  auto Start = Bytes.data() + Offset;
  auto End = Bytes.data() + Bytes.size();
  auto *Ptr = Start;
  while (Ptr < End && *Ptr != '\0') {
    Ptr++;
  }
  return llvm::StringRef(Start, Ptr - Start);
}

llvm::StringRef BytesManager::decodeCStr(int64_t Offset) {
  for (auto &Ent : Bytes) {
    if (Ent.first.containsOffset(Offset)) {
      auto innerOffset = Offset - Ent.first.Start.offset;
      return decodeCStr1(Ent.second, innerOffset);
    }
  }
  return "";
}

std::vector<std::string> decodeFmtStr(llvm::StringRef Format) {
  std::vector<std::string> Ret;
  const char *fmt = Format.data();
  const char *end = fmt + Format.size();
  char ch;

  while (fmt < end && (ch = *fmt++)) {
    if ('%' == ch) {
      switch (ch = *fmt++) {
      /* %% - print out a single %    */
      case '%':
        break;
      /* %c: print out a character    */
      case 'c':
        Ret.push_back("#char");
        break;

      /* %s: print out a string       */
      case 's':
        Ret.push_back("cstr");
        break;

      /* %d: print out an int         */
      case 'd':
        Ret.push_back("#sint");
        break;

      /* %x: print out an int in hex  */
      case 'x':
        Ret.push_back("#sint");
        break;

      case 'f':
        Ret.push_back("#double");
        break;

      case 'e':
        Ret.push_back("#double");
        break;
      }
    }
  }
  return Ret;
}

/*
  "printf": {
    "constraints": [
      "printf.in_0 <= cstr",
      "cstr.load8 <= #char",
      "printf.out <= #sint"
    ],
    "pni_map": {
      "printf": "func p #1",
      "printf.in_0": "ptr p #2",
      "cstr": "ptr p #2",
      "cstr.load8": "int 8 #3",
      "#char": "int 8 #3",
      "printf.out": "int 32 #4",
      "#sint": "int 32 #4",
      "#double": "float 64 #5"
    }
  },
*/

std::shared_ptr<retypd::ConstraintSummary>
buildPrintfSummary(TRContext &Ctx, uint32_t PointerSize,
                   llvm::StringRef FormatStr) {
  std::shared_ptr<retypd::ConstraintSummary> Ret =
      std::make_shared<retypd::ConstraintSummary>();
  Ret->PointerSize = PointerSize;

  auto JO = json::Object(
      {{"constraints", json::Array({json::Value("printf.in_0 <= cstr"),
                                    json::Value("cstr.load8 <= #char"),
                                    json::Value("printf.out <= #sint")})},
       {"pni_map", json::Object({{"printf", "func p #1"},
                                 {"printf.in_0", "ptr p #2"},
                                 {"cstr", "ptr p #2"},
                                 {"cstr.load8", "int 8 #3"},
                                 {"#char", "int 8 #3"},
                                 {"printf.out", "int 32 #4"},
                                 {"#sint", "int 32 #4"},
                                 {"#double", "float 64 #5"}})}});
  json::Array *Constraints = JO.getArray("constraints");
  json::Object *PNIMap = JO.getObject("pni_map");
  // Gen json
  std::vector<std::string> Args = decodeFmtStr(FormatStr);
  for (size_t i = 0; i < Args.size(); i++) {
    auto Arg = Args[i];
    Constraints->push_back(
        json::Value("printf.in_" + std::to_string(i + 1) + " <= " + Arg));
    PNIMap->insert(
        {"printf.in_" + std::to_string(i + 1), *PNIMap->getString(Arg)});
  }

  // create summary from json
  Ret->fromJSON(Ctx, JO);
  return Ret;
}

} // namespace notdec
