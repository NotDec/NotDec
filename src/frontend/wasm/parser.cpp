#include "frontend/wasm/parser.h"


namespace notdec::frontend::wasm {


std::unique_ptr<Context> parse_wasm(BaseContext& llvmCtx, const char *file_name) {
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
    if(this->_func_index != 0 || this->_glob_index != 0) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot add module when globals is not empty" << std::endl;
        std::abort();
    }

    // visit imports & build function index map
    for (Import* import : this->module.imports) {
        switch (import->kind())
        {
        case ExternalKind::Func:
            declareFunc(cast<FuncImport>(import)->func, true);
            break;

        case ExternalKind::Memory:
            // create external memory variable
            declareMemory(cast<MemoryImport>(import)->memory, true);
            break;

        case ExternalKind::Global:
            // set global to external linkage
            visitGlobal(cast<GlobalImport>(import)->global, true);
            break;

        case ExternalKind::Tag:
        case ExternalKind::Table:
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unknown import kind" << std::endl;
            std::abort();
            break;
        }
    }
    // visit global
    for (Global* gl : this->module.globals) {
        visitGlobal(*gl, false);
    }

    // visit memory and data, create memory contentt
    for (Memory* mem: this->module.memories) {
        declareMemory(*mem, false);
    }
    // TODO data
    // iterate without import function
    // see wabt src\wat-writer.cc WatWriter::WriteModule
    for (ModuleField& field : module.fields) {
        if (field.type() != ModuleFieldType::Func) {
            continue;
        }
        Func& func = cast<FuncModuleField>(&field)->func;
        visitFunc(func);
    }
    // for (Func* func: this->module.funcs) {
    //     std::cout << func->name << std::endl;
    //     std::cout << "  " << func->exprs.size() << std::endl;
    // }
    // visit export and change visibility
    for (Export* export_: this->module.exports) {
        Index index;
        // Func* func;
        switch ((*export_).kind)
        {
        case ExternalKind::Func:
            index = module.GetFuncIndex(export_->var);
            // func = module.GetFunc(export_->var);
            // std::cout << "export " << func->name << std::endl;
            // std::cout << "export " << this->funcs[index]->getName().str() << std::endl;
            this->funcs[index]->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            // 之前internal的时候同时自动设置了dso_local
            this->funcs[index]->setDSOLocal(false);
            break;

        case ExternalKind::Table:
            // TODO
            index = module.GetTableIndex(export_->var);
            break;

        case ExternalKind::Memory:
            index = module.GetMemoryIndex(export_->var);
            this->mems[index]->setInitializer(nullptr);
            this->mems[index]->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            break;

        case ExternalKind::Global:
            index = module.GetGlobalIndex(export_->var);
            this->globs[index]->setInitializer(nullptr);
            this->globs[index]->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            break;

        case ExternalKind::Tag:
            index = module.GetTagIndex(export_->var);
            break;
        
        default:
            break;
        }
    }
    // visit elem and create function pointer array
    assert((funcs.size() == _func_index));
}

void Context::visitFunc(wabt::Func& func) {
    using namespace llvm;
    Function* function = declareFunc(func, false);
    setFuncArgName(*function, func.decl.sig);
    auto entryBasicBlock = llvm::BasicBlock::Create(llvmContext, "entry", function);
}

void Context::visitGlobal(wabt::Global& gl, bool isExternal) {
    using namespace llvm;
    // std::string name = "global_" + std::to_string(i) + "_" + gl->name;
    Type* ty = convertType(gl.type);
    GlobalVariable *gv = new GlobalVariable(llvmModule, ty, !gl.mutable_, 
        isExternal ? GlobalValue::LinkageTypes::ExternalLinkage : GlobalValue::LinkageTypes::InternalLinkage, nullptr, gl.name);
    if (!isExternal) {
        Constant* init = visitInitExpr(gl.init_expr);
        if (init == nullptr) {
            // std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Warn: use default InitExpr." << std::endl;
            init = ConstantAggregateZero::get(ty);
        }
        gv->setInitializer(init);
    }
    this->globs.push_back(gv);
    _glob_index ++;
}

llvm::Constant* Context::visitInitExpr(wabt::ExprList& expr) {
    using namespace wabt;
    if (expr.size() != 1) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: InitExpr size != 1." << std::endl;
        std::abort();
    }

    if (expr.front().type() == ExprType::Const) {
        const Const& const_ = cast<ConstExpr>(&expr.front())->const_;
        uint64_t data[2];
        switch (const_.type()) {
        case Type::I32: 
            return llvm::ConstantInt::get(convertType(const_.type()), const_.u32(), false);
        case Type::I64:
            return llvm::ConstantInt::get(convertType(const_.type()), const_.u64(), false);
        case Type::F32:
            return llvm::ConstantFP::get(convertType(const_.type()), llvm::APFloat(ieee_float(const_.f32_bits())));
        case Type::F64:
            return llvm::ConstantFP::get(convertType(const_.type()), llvm::APFloat(ieee_double(const_.f64_bits())));
        case Type::V128:
            data[0] = const_.vec128().u64(0);
            data[1] = const_.vec128().u64(1);
            return llvm::ConstantInt::get(convertType(const_.type()), llvm::APInt(128, llvm::ArrayRef<uint64_t>(data, 2)));
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: InitExpr type unknown: " << const_.type().GetName() << std::endl;
            std::abort();
      }
    } else if (expr.front().type() == ExprType::GlobalGet) {
        // TODO
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: InitExpr with global.get is currently not supported." << std::endl;
        std::abort();
    }
    return nullptr;
}

llvm::GlobalVariable* Context::declareMemory(wabt::Memory& mem, bool isExternal) {
    using namespace llvm;
    uint64_t len = mem.page_limits.initial;
    if (mem.page_limits.has_max) {
        uint64_t max = mem.page_limits.max;
        if (max != len) {
            std::cout << "memory min " << len << " max " << max <<std::endl;
        }
        len = max;
    }
    len = len * 65536;
    ArrayType* ty = ArrayType::get(Type::getInt8Ty(llvmContext), len);
    GlobalVariable *gv = new GlobalVariable(llvmModule, ty, false, 
        isExternal ? GlobalValue::LinkageTypes::ExternalLinkage : GlobalValue::LinkageTypes::InternalLinkage, nullptr, "mem0");
    if (!isExternal) {
        gv->setInitializer(ConstantAggregateZero::get(ty));
    }
    this->mems.push_back(gv);
    _mem_index ++;
    return gv;
}

llvm::Function* Context::declareFunc(wabt::Func& func, bool isExternal) {
    using namespace llvm;
    FunctionType* funcType = convertFuncType(func.decl.sig);
    Function* function = Function::Create(
            funcType,
            isExternal ? Function::ExternalLinkage : Function::InternalLinkage,
            func.name,
            llvmModule);
    this->funcs.push_back(function);
    this->_func_index ++;
    return function;
}

void Context::setFuncArgName(llvm::Function& func, const wabt::FuncSignature& decl) {
    using namespace llvm;
    int argSize = decl.GetNumParams();
    for (unsigned int i=0;i<argSize;i++) {
        const std::string& name = decl.GetParamType(i).GetName();
        func.getArg(i)->setName(name);
        // std::cout << func.getArg(i)->getName().str() << std::endl;
    }
}

llvm::FunctionType* Context::convertFuncType(const wabt::FuncSignature& decl) {
    using namespace llvm;
    Type** llvmArgTypes;
    size_t numParameters = decl.param_types.size();
    llvmArgTypes = (Type**)alloca(sizeof(Type*) * numParameters);
    int i = 0;
    for (const wabt::Type& pt: decl.param_types) {
        llvmArgTypes[i] = convertType(pt);
        i++;
    }
    if (decl.GetNumResults() > 1) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Multi result is not currently supported." << std::endl;
        std::abort();
    }
    Type* llvmReturnType;
    if (decl.GetNumResults() == 0) {
        llvmReturnType = Type::getVoidTy(llvmContext);
    } else {
        llvmReturnType = convertType(decl.GetResultType(0));
    }
    return FunctionType::get(
        llvmReturnType, ArrayRef<Type*>(llvmArgTypes, numParameters), false);
}

llvm::Type* Context::convertType(const wabt::Type& ty) {
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