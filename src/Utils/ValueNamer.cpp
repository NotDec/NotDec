#include "notdec/Utils/ValueNamer.h"

namespace notdec {

std::set<size_t> TraceIds;

ValueNamer ValueNamer::Instance = ValueNamer();
const char *ValueNamer::DefaultPrefix = "v_";
const char *ValueNamer::FuncPrefix = "func_";
const char *ValueNamer::PhiPrefix = "phi_";
const char *ValueNamer::SelectPrefix = "select_";
const char *ValueNamer::NewPrefix = "new_";
const char *ValueNamer::AddPrefix = "add_";
const char *ValueNamer::SubPrefix = "sub_";
const char *ValueNamer::StackPrefix = "stack_";
const char *ValueNamer::AllocaPrefix = "alloca_";
const char *ValueNamer::LoadPrefix = "load_";
const char *ValueNamer::StorePrefix = "store_";

}
