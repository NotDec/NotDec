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
    // std::unique_ptr<FileStream> s_log_stream = FileStream::CreateStderr();
    ReadBinaryOptions options(s_features, nullptr,// s_log_stream.get(),
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
    using namespace llvm;
    Module& mod = this->llvmCtx.mod;

    // visit global
    if(this->globs.size() != 0) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot add module when globals is not empty" << std::endl;
        std::abort();
    }
    int i = 0; // index
    for (wabt::Global* gl : this->module.globals) {
        std::string name = "global_" + std::to_string(i) + "_" + gl->name;
        GlobalVariable *gv = new GlobalVariable(mod, convertType(gl->type), !gl->mutable_, 
            GlobalValue::LinkageTypes::InternalLinkage, nullptr, name);
        this->globs.push_back(gv);
        i += 1;
    }
    // visit imports & build function index map
    
    // visit exports and w x
}

llvm::Type* Context::convertType(wabt::Type& ty) {
    using namespace llvm;
    LLVMContext& ctx = this->llvmCtx.context;
    switch(ty) {
        case wabt::Type::I32:
            return Type::getInt32Ty(ctx);
        case wabt::Type::I64:
            return Type::getInt64Ty(ctx);
        case wabt::Type::F32:
            return Type::getFloatTy(ctx);
        case wabt::Type::F64:
            return Type::getDoubleTy(ctx);
        case wabt::Type::V128:
            return Type::getInt128Ty(ctx);
        case wabt::Type::Void:
            return Type::getVoidTy(ctx);
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot convert type: " << ty.GetName() << std::endl;
            std::abort();
    }
}

}