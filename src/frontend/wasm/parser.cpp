


#include <iostream>

#include "frontend/wasm/parser.h"


namespace notdec::frontend::wasm {


Context *
parse_wasm(const char *file_name) {
    using namespace wabt;
    std::vector<uint8_t> file_data;
    Result result = ReadFile(file_name, &file_data);
    if (Succeeded(result)) {
        Errors errors;
        Module module;
        const bool kStopOnFirstError = true;
        Features s_features;
        std::unique_ptr<FileStream> s_log_stream = FileStream::CreateStderr();
        ReadBinaryOptions options(s_features, s_log_stream.get(),
                                true, kStopOnFirstError,
                                true);
        result = ReadBinaryIr(file_name, file_data.data(), file_data.size(),
                            options, &errors, &module);
        if (!Succeeded(result)) {
            std::cerr << "Read wasm file failed." << std::endl;
            return nullptr;
        }
        bool s_validate = true;
        if (s_validate) {
            ValidateOptions options(s_features);
            result = ValidateModule(&module, &errors, options);
            if (!Succeeded(result)) {
                std::cerr << "Wasm validation failed." << std::endl;
                return nullptr;
            }
        }
        // TODO
    }
    
    return nullptr;
}


}