#include "frontend/wasm/parser.h"


namespace notdec::frontend::wasm {


std::unique_ptr<Context> 
parse_wasm(BaseContext& llvmCtx, const char *file_name) {
    using namespace wabt;
    std::vector<uint8_t> file_data;
    Result result = ReadFile(file_name, &file_data);
    
    // Context ctx(llvmCtx);
    std::unique_ptr<Context> ret = std::make_unique<Context>(llvmCtx);
    if (!Succeeded(result)) {
        std::cerr << "Read wasm file failed." << std::endl;
        return std::unique_ptr<Context>(nullptr);
    }
    Errors errors;
    const bool kStopOnFirstError = true;
    Features s_features;
    std::unique_ptr<FileStream> s_log_stream = FileStream::CreateStderr();
    ReadBinaryOptions options(s_features, s_log_stream.get(),
                            true, kStopOnFirstError,
                            true);
    result = ReadBinaryIr(file_name, file_data.data(), file_data.size(),
                        options, &errors, &(ret->module));
    if (!Succeeded(result)) {
        std::cerr << "Read wasm file failed." << std::endl;
        return std::unique_ptr<Context>(nullptr);
    }
    bool s_validate = true;
    if (s_validate) {
        ValidateOptions options(s_features);
        result = ValidateModule(&(ret->module), &errors, options);
        if (!Succeeded(result)) {
            std::cerr << "Wasm validation failed." << std::endl;
            return std::unique_ptr<Context>(nullptr);
        }
    }
    // TODO
    ret->visitModule();
    return ret;
}

void Context::visitModule() {
    // visit global
    // visit imports & build function index map
    // visit exports and w x

}

}