#ifndef _FRONTEND_WASM_PARSER_H_
#define _FRONTEND_WASM_PARSER_H_


// wabt header
#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/ir.h"
#include "src/stream.h"
#include "src/validator.h"

#include "frontend/context.h"

namespace notdec::frontend::wasm {

struct Context {
    BaseContext llvmCtx;
    const wabt::Module& module;
};

Context *
parse_wasm(const char *file_name);

}
#endif