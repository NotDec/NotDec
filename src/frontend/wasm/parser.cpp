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
    using namespace wabt;
    // change module name from file name to wasm module name if there is
    if (!module.name.empty()) {
        llvmModule.setModuleIdentifier(module.name);
    }

    // visit global
    if(this->globs.size() != 0) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot add module when globals is not empty" << std::endl;
        std::abort();
    }
    Index i = 0;
    for (Global* gl : this->module.globals) {
        visitGlobal(gl, i);
        i++;
    }
    // visit imports & build function index map
    for (Import* import : this->module.imports) {
        switch (import->kind())
        {
        case ExternalKind::Func:
            // visitImportFunc(&(cast<FuncImport>(import)->func));
            break;
        case ExternalKind::Table:
            break;

        case ExternalKind::Memory:
            break;

        case ExternalKind::Global:
            break;

        case ExternalKind::Tag:
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unknown import kind" << std::endl;
            std::abort();
            break;
        }
    }
    // iterate without import function
    // see wabt src\wat-writer.cc WatWriter::WriteModule
    for (ModuleField& field : module.fields) {
        if (field.type() != ModuleFieldType::Func) {
            continue;
        }
        Func& func = cast<FuncModuleField>(&field)->func;
        // std::cout << func.name << std::endl;
    }
    // for (Func* func: this->module.funcs) {
    //     std::cout << func->name << std::endl;
    //     std::cout << "  " << func->exprs.size() << std::endl;
    // }
    // visit exports and w x
}

void Context::visitGlobal(wabt::Global* gl, wabt::Index index) {
    using namespace llvm;
    // std::string name = "global_" + std::to_string(i) + "_" + gl->name;
    GlobalVariable *gv = new GlobalVariable(llvmModule, convertType(gl->type), !gl->mutable_, 
        GlobalValue::LinkageTypes::InternalLinkage, nullptr, gl->name);
    this->globs.push_back(gv);
}

void Context::visitImportFunc(wabt::Func* func) {
    using namespace llvm;
    FunctionType* funcType = convertFuncType(func->decl.sig);
    Function* function = Function::Create(
			funcType,
			Function::ExternalLinkage,
			func->name,
			llvmModule);
    this->funcs.push_back(function);
    this->_func_index ++;
}

llvm::FunctionType* Context::convertFuncType(const wabt::FuncSignature& decl) {
    // for (:func_sig.param_types)
    // for (:func_sig.result_types)
    return nullptr;
}

llvm::Type* Context::convertType(wabt::Type& ty) {
    using namespace llvm;
    switch(ty) {
        case wabt::Type::I32:
            return Type::getInt32Ty(llvmContext);
        case wabt::Type::I64:
            return Type::getInt64Ty(llvmContext);
        case wabt::Type::F32:
            return Type::getFloatTy(llvmContext);
        case wabt::Type::F64:
            return Type::getDoubleTy(llvmContext);
        case wabt::Type::V128:
            return Type::getInt128Ty(llvmContext);
        case wabt::Type::Void:
            return Type::getVoidTy(llvmContext);
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot convert type: " << ty.GetName() << std::endl;
            std::abort();
    }
}

}