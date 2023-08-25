#include "optimizers/fact-generator.h"
#include <string>
#include <sstream>

namespace notdec::frontend::optimizers {

template<typename Value, typename... Values>
std::string to_fact_str ( Value v, Values... vs )
{
    std::ostringstream oss;
    using expander = int[];
    oss << v; // first
    (void) expander{ 0, (oss << '\t' << vs, void(), 0)... };
    return oss.str() + '\n';
}

void FactGenerator::generate(llvm::Module &mod) {
    FactGenerator fg(mod);
    fg.emit_gvs();
    fg.emit_functions();
}

void FactGenerator::emit_gvs() {
    for (auto &gv : mod.globals()) {
        auto name = gv.getNameOrAsOperand();
        auto storageType = getLinkageName(gv.getLinkage());
        auto type = printType(gv.getValueType());
        append_fact("global", to_fact_str(name, storageType, type));
    }
}

void FactGenerator::append_fact(const char* key, std::string to_append) {
    // https://stackoverflow.com/questions/1409454/c-map-find-to-possibly-insert-how-to-optimize-operations
    typedef std::map<const char*,std::string> M;
    typedef M::iterator I;
    std::pair<I, bool> const& r=facts.insert(M::value_type(key, to_append));
    if (r.second) {
        // value was inserted; now my_map[foo_obj]="some value"
    } else {
        // value wasn't inserted because my_map[foo_obj] already existed.
        // note: the old value is available through r.first->second
        // and may not be "some value"
        r.first->second.append(to_append);
    }
}

void FactGenerator::emit_functions() {
    for(auto &f: mod.functions()) {
        
    }
}

std::string printType(Type* ty) {
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    ty->print(rso);
    return rso.str();
}

std::string getLinkageName(GlobalValue::LinkageTypes LT) {
  switch (LT) {
  case GlobalValue::ExternalLinkage:
    return "external";
  case GlobalValue::PrivateLinkage:
    return "private";
  case GlobalValue::InternalLinkage:
    return "internal";
  case GlobalValue::LinkOnceAnyLinkage:
    return "linkonce";
  case GlobalValue::LinkOnceODRLinkage:
    return "linkonce_odr";
  case GlobalValue::WeakAnyLinkage:
    return "weak";
  case GlobalValue::WeakODRLinkage:
    return "weak_odr";
  case GlobalValue::CommonLinkage:
    return "common";
  case GlobalValue::AppendingLinkage:
    return "appending";
  case GlobalValue::ExternalWeakLinkage:
    return "extern_weak";
  case GlobalValue::AvailableExternallyLinkage:
    return "available_externally";
  }
  llvm_unreachable("invalid linkage");
}

}

