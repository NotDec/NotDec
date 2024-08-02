
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/Schema.h"
#include "Utils/Range.h"
#include <cassert>
#include <iostream>
#include <string>

namespace notdec::retypd {

// Keeps in sync with the printing.

// get the size of the first utf-8 code point.
std::size_t GetFirst(const llvm::StringRef &text, std::size_t start) {
  if (start >= text.size())
    return 0;
  std::size_t length = 1;
  while (start + length < text.size() &&
         ((text[start + length] & 0b11000000) == 0b10000000)) {
    ++length;
  }
  return length;
}

llvm::StringRef skipWhitespace(llvm::StringRef str) {
  while (!str.empty() && std::isspace(str.front())) {
    str = str.drop_front(GetFirst(str));
  }
  return str;
}

bool is_dtv_separator(llvm::StringRef str) {
  char c = str.front();
  return std::isspace(c) || c == '<' || c == '=' || str.startswith("⊑");
}

bool is_separator(llvm::StringRef str) {
  char c = str.front();
  return std::isspace(c) || c == '.' || c == '<' || c == '=' ||
         str.startswith("⊑");
}

ParseResultT<llvm::StringRef> parseIdentifier(llvm::StringRef Str) {
  Str = skipWhitespace(Str);
  llvm::StringRef Rest = Str;
  std::size_t end = 0;
  while (end < Str.size() && !is_separator(Rest)) {
    end += GetFirst(Rest);
    Rest = Str.substr(end);
  }
  return {Rest, Str.substr(0, end)};
}

/// Ensure that the parsed string length is not zero.
ParseResultT<llvm::StringRef> mustParseIdentifier(llvm::StringRef str) {
  auto [rest, RName] = parseIdentifier(str);
  if (!RName.isOk()) {
    return {rest, RName};
  }
  llvm::StringRef &Name = RName.get();
  if (Name.size() == 0) {
    return {rest, "mustParseIdentifier: Expect name: " + str.substr(0, 10)};
  }
  return {rest, Name};
}

ParseResultT<int32_t> parseI32(llvm::StringRef str) {
  bool negative = false;
  if (str.consume_front("-")) {
    negative = true;
  }
  std::size_t end = 0;
  while (end < str.size() && std::isdigit(str[end])) {
    ++end;
  }
  if (end == 0) {
    return {str, "parseI32: Expect number: " + str.substr(0, 10)};
  } else {
    int32_t num = std::stoi(str.substr(0, end).str());
    return {str.substr(end), negative ? -num : num};
  }
}

ParseResultT<uint32_t> parseU32(llvm::StringRef str) {
  std::size_t end = 0;
  while (end < str.size() && std::isdigit(str[end])) {
    ++end;
  }
  if (end == 0) {
    return {str, "parseU32: Expect number: " + str.substr(0, 10)};
  } else {
    uint32_t num = std::stoul(str.substr(0, end).str());
    return {str.substr(end), num};
  }
}

ParseResultT<InLabel> parseInLabel(llvm::StringRef str) {
  str = skipWhitespace(str);
  auto [rest, RName] = mustParseIdentifier(str);
  // unwrap name
  if (!RName.isOk()) {
    return {str, RName.msg()};
  }
  llvm::StringRef &Name = RName.get();
  if (Name.consume_front("in_")) {
    return {rest, InLabel{Name.str()}};
  } else {
    return {str, "parseInLabel: Expect in_: " + str.substr(0, 10)};
  }
}

ParseResultT<OutLabel> parseOutLabel(llvm::StringRef str) {
  str = skipWhitespace(str);
  auto [rest, RName] = mustParseIdentifier(str);
  // unwrap name
  if (!RName.isOk()) {
    return {rest, RName.msg()};
  }
  llvm::StringRef &Name = RName.get();
  if (Name.consume_front("out_")) {
    return {rest, OutLabel{Name.str()}};
  } else if (Name.consume_front("out") && Name.empty()) {
    return {rest, OutLabel{}};
  } else {
    return {str, "parseOutLabel: Expect out or out_: " + str.substr(0, 10)};
  }
}

ParseResultT<ArrayOffset> parseArrayOffset(llvm::StringRef str) {
  str = skipWhitespace(str);
  if (!str.consume_front("+")) {
    return {str, "parseArrayOffset: Expect +: " + str.substr(0, 10)};
  }
  str = skipWhitespace(str);
  auto [rest1, RSize] = parseU32(str);
  if (!RSize.isOk()) {
    return {str, RSize.msg()};
  }
  decltype(ArrayOffset::Limit) Limit = -1;
  if (rest1.consume_front("[")) {
    auto [rest2, RLimit] = parseI32(rest1);
    if (!RLimit.isOk()) {
      return {str, RLimit.msg()};
    }
    Limit = RLimit.get();
  }
  return {rest1, ArrayOffset{RSize.get(), Limit}};
}

ParseResultT<OffsetLabel> parseOffsetLabel(llvm::StringRef str) {
  str = skipWhitespace(str);
  llvm::StringRef rest = str;

  decltype(OffsetRange::offset) Offset = 0;
  if (!rest.consume_front("@")) {
    return {str, "parseOffsetLabel: Expect @: " + str.substr(0, 10)};
  }
  auto [rest2, ROffset] = parseU32(rest);
  if (!ROffset.isOk()) {
    return {str, ROffset.msg()};
  }
  Offset = ROffset.get();
  rest = rest2;

  OffsetRange ret = OffsetRange{.offset = Offset};
  while (rest.size() > 0 && rest.front() == '+') {
    auto [rest3, RArray] = parseArrayOffset(rest);
    if (!RArray.isOk()) {
      return {str, RArray.msg()};
    }
    ret.access.push_back(RArray.get());
    rest = rest3;
  }
  return {rest, OffsetLabel{.range = ret}};
}

ParseResultT<LoadLabel> parseLoad(llvm::StringRef str) {
  str = skipWhitespace(str);
  if (!str.consume_front("load")) {
    return {str, "parseLoad: Expect load: " + str.substr(0, 10)};
  }
  auto [rest, RSize] = parseU32(str);
  if (!RSize.isOk()) {
    return {str, RSize.msg()};
  }
  str = rest;
  return {str, LoadLabel{.Size = RSize.get()}};
}

ParseResultT<StoreLabel> parseStore(llvm::StringRef str) {
  str = skipWhitespace(str);
  if (!str.consume_front("store")) {
    return {str, "parseLoad: Expect store: " + str.substr(0, 10)};
  }
  auto [rest, RSize] = parseU32(str);
  if (!RSize.isOk()) {
    return {str, RSize.msg()};
  }
  str = rest;
  return {str, StoreLabel{.Size = RSize.get()}};
}

ParseResultT<FieldLabel> parseFieldLabel(llvm::StringRef str) {
  str = skipWhitespace(str);
  auto [rest, RIn] = parseInLabel(str);
  if (RIn.isOk()) {
    return {rest, FieldLabel{RIn.get()}};
  }
  auto [rest1, ROut] = parseOutLabel(str);
  if (ROut.isOk()) {
    return {rest1, FieldLabel{ROut.get()}};
  }
  auto [rest2, RDeref] = parseOffsetLabel(str);
  if (RDeref.isOk()) {
    return {rest2, FieldLabel{RDeref.get()}};
  }
  auto [rest3, RLoad] = parseLoad(str);
  if (RLoad.isOk()) {
    return {rest3, FieldLabel{RLoad.get()}};
  }
  auto [rest4, RStore] = parseStore(str);
  if (RStore.isOk()) {
    return {rest4, FieldLabel{RStore.get()}};
  }
  return {str, "parseFieldLabel: Expect field label: " + str.substr(0, 10)};
}

ParseResultT<DerivedTypeVariable>
parseDerivedTypeVariable(llvm::StringRef str) {
  str = skipWhitespace(str);
  auto [rest, RName] = mustParseIdentifier(str);
  if (!RName.isOk()) {
    return {rest, RName.msg()};
  }
  llvm::StringRef &Name = RName.get();
  std::deque<FieldLabel> labels;
  rest = skipWhitespace(rest);
  while (!rest.empty() && !is_dtv_separator(rest)) {
    if (!rest.consume_front(".")) {
      return {rest,
              "parseDerivedTypeVariable: Expect .: " + rest.substr(0, 10)};
    }
    auto [rest1, RField] = parseFieldLabel(rest);
    if (!RField.isOk()) {
      return {rest, RField.msg()};
    }
    auto &Field = RField.get();
    labels.push_back(Field);
    rest = rest1;
    rest = skipWhitespace(rest);
  }
  return {rest, DerivedTypeVariable{Name.str(), labels}};
}

ParseResultT<TypeVariable> parseTypeVariable(llvm::StringRef str) {
  str = skipWhitespace(str);
  if (str.consume_front("#Int#")) {
    assert(false && "Not implemented");
  } else if (str.consume_front("#")) {
    auto [rest, RName] = mustParseIdentifier(str);
    if (!RName.isOk()) {
      return {rest, RName.msg()};
    }
    return {rest, TypeVariable::CreatePrimitive(RName.get().str())};
  } else {
    auto [rest, RDtv] = parseDerivedTypeVariable(str);
    if (!RDtv.isOk()) {
      return {rest, RDtv.msg()};
    }
    return {rest, TypeVariable{RDtv.get()}};
  }
}

ParseResultT<SubTypeConstraint> parseSubTypeConstraint(llvm::StringRef str) {
  str = skipWhitespace(str);
  auto [rest, RSub] = parseTypeVariable(str);
  if (!RSub.isOk()) {
    return {rest, RSub.msg()};
  }
  rest = skipWhitespace(rest);
  if (!(rest.consume_front("⊑") || rest.consume_front("<="))) {
    return {rest, "parseSubTypeConstraint: Expect <= :" + rest.substr(0, 10)};
  }
  rest = skipWhitespace(rest);
  auto [rest1, RSup] = parseTypeVariable(rest);
  if (!RSup.isOk()) {
    return {rest, RSup.msg()};
  }
  return {rest1, SubTypeConstraint{{RSub.get()}, {RSup.get()}}};
}

} // namespace notdec::retypd
