#ifndef _NOTDEC_RETYPD_PARSER_H_
#define _NOTDEC_RETYPD_PARSER_H_

#include "Schema.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <utility>
#include <variant>
namespace notdec::retypd {

template <typename T> using ResultT = std::variant<T, llvm::Twine>;

// TODO Refactor to use expected.
// TODO refactor to use a C++ parser combinator like
// https://github.com/foonathan/lexy
template <typename T> struct Result {
  ResultT<T> inner;
  Result(const T &inner) : inner(inner) {}
  Result(const llvm::Twine &msg) : inner(msg) {}
  bool isOk() const { return std::holds_alternative<T>(inner); }
  bool isErr() const { return std::holds_alternative<llvm::Twine>(inner); }
  llvm::Twine msg() const { return std::get<llvm::Twine>(inner); }
  T &get() { return std::get<T>(inner); }
  T &operator*() { return get(); }
};

template <typename T>
using ParseResultT = std::pair<llvm::StringRef, Result<T>>;

std::size_t GetFirst(const llvm::StringRef &text, std::size_t start = 0);
llvm::StringRef skipWhitespace(llvm::StringRef str);
bool is_separator(llvm::StringRef str);
ParseResultT<llvm::StringRef> parseIdentifier(llvm::StringRef str);
ParseResultT<llvm::StringRef> mustParseIdentifier(llvm::StringRef str);
ParseResultT<int32_t> parseI32(llvm::StringRef str);
ParseResultT<InLabel> parseInLabel(llvm::StringRef str);
ParseResultT<OutLabel> parseOutLabel(llvm::StringRef str);
ParseResultT<OffsetLabel> parseOffsetLabel(llvm::StringRef str);
ParseResultT<LoadLabel> parseLoad(llvm::StringRef str);
ParseResultT<StoreLabel> parseStore(llvm::StringRef str);
ParseResultT<FieldLabel> parseFieldLabel(llvm::StringRef str);
ParseResultT<DerivedTypeVariable> parseDerivedTypeVariable(llvm::StringRef str);
ParseResultT<SubTypeConstraint> parseSubTypeConstraint(llvm::StringRef str);

} // namespace notdec::retypd

#endif
