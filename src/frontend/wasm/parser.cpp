#include "frontend/wasm/parser.h"
#include "frontend/wasm/parser-block.h"


namespace notdec::frontend::wasm {

std::unique_ptr<Context> parse_wat(BaseContext& llvmCtx, const char *file_name) {
    using namespace wabt;
    std::vector<uint8_t> file_data;
    Result result = ReadFile(file_name, &file_data);
    std::unique_ptr<WastLexer> lexer = WastLexer::CreateBufferLexer(
      file_name, file_data.data(), file_data.size());

    
    if (!Succeeded(result)) {
        std::cerr << "Read wat file failed." << std::endl;
        return std::unique_ptr<Context>(nullptr);
    }
    // Context ctx(llvmCtx);
    std::unique_ptr<Context> ret = std::make_unique<Context>(llvmCtx);

    Errors errors;
    Features s_features;
    // std::unique_ptr<FileStream> s_log_stream = FileStream::CreateStderr();
    WastParseOptions options(s_features);
    result = ParseWatModule(lexer.get(), (&(ret->module)), &errors, &options);
    if (!Succeeded(result)) {
        std::cerr << "Read wat file failed." << std::endl;
        return std::unique_ptr<Context>(nullptr);
    }
    bool s_validate = true;
    if (s_validate) {
        ValidateOptions options(s_features);
        result = ValidateModule(ret->module.get(), &errors, options);
        if (!Succeeded(result)) {
            std::cerr << "Wat validation failed." << std::endl;
            return std::unique_ptr<Context>(nullptr);
        }
    }
    // do generation
    ret->visitModule();
    return ret;
}

std::unique_ptr<Context> parse_wasm(BaseContext& llvmCtx, const char *file_name) {
    using namespace wabt;
    // 这部分代码来自WABT，解析wasm文件
    std::vector<uint8_t> file_data;
    Result result = ReadFile(file_name, &file_data);
    
    // Context ctx(llvmCtx);
    std::unique_ptr<Context> ret = std::make_unique<Context>(llvmCtx);
    ret->module = std::make_unique<Module>();
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
                        options, &errors, ret->module.get());
    if (!Succeeded(result)) {
        std::cerr << "Read wasm file failed." << std::endl;
        return std::unique_ptr<Context>(nullptr);
    }
    bool s_validate = true;
    if (s_validate) {
        ValidateOptions options(s_features);
        result = ValidateModule(ret->module.get(), &errors, options);
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
    if (!module->name.empty()) {
        llvmModule.setModuleIdentifier(module->name);
    }
    if(this->_func_index != 0 || this->_glob_index != 0) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot add module when globals is not empty" << std::endl;
        std::abort();
    }

    // visit imports & build function index map
    for (Import* import : this->module->imports) {
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
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unknown import kind: " << g_kind_name[static_cast<size_t>(import->kind())] << std::endl;
            // std::abort();
            break;
        }
    }
    // visit global
    for (Global* gl : this->module->globals) {
        visitGlobal(*gl, false);
    }

    // visit memory and data, create memory content
    // for (Memory* mem: this->module->memories) {
    //     declareMemory(*mem, false);
    // }
    for (ModuleField& field : module->fields) {
        // wabt\src\wat-writer.cc WatWriter::WriteModule
        if (field.type() != ModuleFieldType::Memory) {
            continue;
        }
        Memory& mem = cast<MemoryModuleField>(&field)->memory;
        declareMemory(mem, false);
    }

    for (ModuleField& field : module->fields) {
        if (field.type() != ModuleFieldType::DataSegment) {
            continue;
        }
        DataSegment& ds = cast<DataSegmentModuleField>(&field)->data_segment;
        visitDataSegment(ds);
    }

    // TODO data

    std::vector<llvm::Function*> nonImportFuncs;
    // iterate without import function
    // see wabt src\wat-writer.cc WatWriter::WriteModule
    for (ModuleField& field : module->fields) {
        if (field.type() != ModuleFieldType::Func) {
            continue;
        }
        Func& func = cast<FuncModuleField>(&field)->func;
        llvm::Function* function = declareFunc(func, false);
        // if (baseCtx.opt.test_mode) {
        //     function->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
        // }
        nonImportFuncs.push_back(function); 
    }
    std::size_t i = 0;
    for (ModuleField& field : module->fields) {
        if (field.type() != ModuleFieldType::Func) {
            continue;
        }
        Func& func = cast<FuncModuleField>(&field)->func;
        visitFunc(func, nonImportFuncs.at(i));
        i++;
    }
    // for (Func* func: this->module.funcs) {
    //     std::cout << func->name << std::endl;
    //     std::cout << "  " << func->exprs.size() << std::endl;
    // }
    // visit export and change visibility
    for (Export* export_: this->module->exports) {
        Index index;
        // Func* func;
        switch ((*export_).kind)
        {
        case ExternalKind::Func:
            index = module->GetFuncIndex(export_->var);
            // func = module.GetFunc(export_->var);
            // std::cout << "export " << func->name << std::endl;
            // std::cout << "export " << this->funcs[index]->getName().str() << std::endl;
            this->funcs[index]->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            // 之前internal的时候同时自动设置了dso_local
            this->funcs[index]->setDSOLocal(false);
            break;

        case ExternalKind::Table:
            // TODO
            index = module->GetTableIndex(export_->var);
            break;

        case ExternalKind::Memory:
            index = module->GetMemoryIndex(export_->var);
            // this->mems[index]->setInitializer(nullptr);
            this->mems[index]->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            this->mems[index]->setDSOLocal(false);
            break;

        case ExternalKind::Global:
            index = module->GetGlobalIndex(export_->var);
            // this->globs[index]->setInitializer(nullptr);
            this->globs[index]->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            this->globs[index]->setDSOLocal(false);
            break;

        case ExternalKind::Tag:
            index = module->GetTagIndex(export_->var);
            break;

        default:
            break;
        }
    }
    // visit elem and create function pointer array
    assert((this->funcs.size() == _func_index));
}

llvm::GlobalVariable* Context::visitDataSegment(wabt::DataSegment& ds) {
    using namespace llvm;
    wabt::Index index = module->GetMemoryIndex(ds.memory_var);
    if (index >= _mem_index) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: data section mem ref out of bound." << index << std::endl;
        std::abort();
    }
    // LLVM side mem manipulation
    GlobalVariable* mem = this->mems.at(index);
    Type* memty = mem->getValueType();
    assert(!mem->hasInitializer() || isa<ConstantAggregateZero>(* (mem->getInitializer())));
    llvm::Constant* offset = visitInitExpr(ds.offset);
    Constant* init = createMemInitializer(llvmContext, memty, unwrapIntConstant(offset), ds.data);
    mem->setInitializer(init);
    return mem;
}

const std::string LOCAL_PREFIX = "_local_";
const std::string PARAM_PREFIX = "_param_";

void Context::visitFunc(wabt::Func& func, llvm::Function* function) {
    using namespace llvm;
    
    BasicBlock* allocaBlock = llvm::BasicBlock::Create(llvmContext, "allocator", function);

    // auto returnPHIs = createPHIs(returnBlock, func.decl.sig.result_types);
    
    IRBuilder<> irBuilder(llvmContext);
    irBuilder.SetInsertPoint(allocaBlock);
    // std::unique_ptr<std::vector<llvm::Value*>> locals = std::make_unique<std::vector<llvm::Value*>>();
    std::vector<llvm::Value*> locals = std::vector<llvm::Value*>();

    // handle locals (params)
    Function::arg_iterator llvmArgIt = function->arg_begin();
    wabt::Index numParam = func.GetNumParams();
    for (wabt::Index i = 0;i<numParam;i++) {
        AllocaInst* alloca = irBuilder.CreateAlloca(convertType(llvmContext, func.GetParamType(i)), nullptr, PARAM_PREFIX + std::to_string(i));
        locals.push_back(alloca);
        irBuilder.CreateStore(&*llvmArgIt, alloca);
        ++llvmArgIt;
    }
    // handle locals
    wabt::Index numLocal = func.GetNumLocals();
    for (wabt::Index i = 0;i<numLocal;i++) {
        AllocaInst* alloca = irBuilder.CreateAlloca(convertType(llvmContext, func.local_types[i]), nullptr, LOCAL_PREFIX + std::to_string(numParam + i));
        locals.push_back(alloca);
        irBuilder.CreateStore(convertZeroValue(llvmContext, func.local_types[i]), alloca);
    }

    // alloca -> entry, ensure alloca will only execute once.
    BasicBlock* entryBasicBlock = llvm::BasicBlock::Create(llvmContext, "entry", function);
    BasicBlock* returnBlock = llvm::BasicBlock::Create(llvmContext, "return", function);
    irBuilder.CreateBr(entryBasicBlock);

    // create implicit entry -> return;
    irBuilder.SetInsertPoint(entryBasicBlock);
    irBuilder.SetInsertPoint(irBuilder.CreateBr(returnBlock)); // insert before jump

    BlockContext bctx(*this, *function, irBuilder, std::move(locals));
    bctx.visitBlock(wabt::LabelType::Func, entryBasicBlock, returnBlock, func.decl, func.exprs);

    // handle implicit return
    // TODO MultiValue
    irBuilder.SetInsertPoint(returnBlock);
    if (func.GetNumResults() == 1) {
        irBuilder.CreateRet(bctx.stack.back()); bctx.stack.pop_back();
    } else {
        assert(func.GetNumResults() == 0);
        irBuilder.CreateRetVoid();
    }
}

void Context::visitGlobal(wabt::Global& gl, bool isExternal) {
    using namespace llvm;
    // std::string name = "global_" + std::to_string(i) + "_" + gl->name;
    Type* ty = convertType(llvmContext, gl.type);
    GlobalVariable *gv = new GlobalVariable(llvmModule, ty, !gl.mutable_, 
        isExternal ? GlobalValue::LinkageTypes::ExternalLinkage : GlobalValue::LinkageTypes::InternalLinkage, nullptr, gl.name.empty() ? "G" + std::to_string(_glob_index) : gl.name);
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
        return visitConst(llvmContext, const_);
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
    FunctionType* funcType = convertFuncType(llvmContext, func.decl.sig);
    Function* function = Function::Create(
            funcType,
            isExternal ? Function::ExternalLinkage : Function::InternalLinkage,
            baseCtx.opt.recompile ? removeDollar(func.name): func.name,
            llvmModule);
    this->funcs.push_back(function);
    this->_func_index ++;
    setFuncArgName(*function, func.decl.sig);
    return function;
}

llvm::Constant* createMemInitializer(llvm::LLVMContext& llvmContext, llvm::Type* memty, uint64_t offset, std::vector<uint8_t> data) {
    using namespace llvm;
    uint64_t memsize = cast<ArrayType>(*memty).getNumElements();
    assert(data.size() + offset);
    // TODO store buffer somewhere. create a struct for memory?
    uint8_t* buffer = new uint8_t[memsize];
    // copy data
    ::memcpy(buffer+offset, data.data(), data.size() * sizeof(uint8_t));
    return ConstantDataArray::getRaw(StringRef((char*)buffer, memsize), memsize, Type::getInt8Ty(llvmContext));
}

std::string removeDollar(std::string name) {
    if (name.length() == 0) {
        return name;
    }
    if (name.at(0) == '$') {
        return name.substr(1);
    }
    return name;
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

llvm::Function* Context::findFunc(wabt::Var& var) {
    wabt::Index ind;
    if (var.is_index()) {
        ind = var.index();
    } else {
        ind = module->func_bindings.FindIndex(var);
    }
    return funcs.at(ind);
}

llvm::FunctionType* convertFuncType(llvm::LLVMContext& llvmContext, const wabt::FuncSignature& decl) {
    using namespace llvm;
    Type** llvmArgTypes;
    size_t numParameters = decl.param_types.size();
    llvmArgTypes = (Type**)alloca(sizeof(Type*) * numParameters);
    int i = 0;
    for (const wabt::Type& pt: decl.param_types) {
        llvmArgTypes[i] = convertType(llvmContext, pt);
        i++;
    }
    // TODO multi return
    if (decl.GetNumResults() > 1) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Multi result is currently not supported." << std::endl;
        std::abort();
    }
    Type* llvmReturnType = convertReturnType(llvmContext, decl);
    return FunctionType::get(
        llvmReturnType, ArrayRef<Type*>(llvmArgTypes, numParameters), false);
}

llvm::Type* convertReturnType(llvm::LLVMContext& llvmContext, const wabt::FuncSignature& decl) {
    using namespace llvm;
    Type* llvmReturnType;
    if (decl.GetNumResults() == 0) {
        llvmReturnType = Type::getVoidTy(llvmContext);
    } else {
        llvmReturnType = convertType(llvmContext, decl.GetResultType(0));
    }
    return llvmReturnType;
}

// TODO 从类里提出来
llvm::Type* convertType(llvm::LLVMContext& llvmContext, const wabt::Type& ty) {
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

llvm::Constant* convertZeroValue(llvm::LLVMContext& llvmContext, const wabt::Type& ty) {
    using namespace llvm;
    switch(ty) {
        case wabt::Type::I32:
            return ConstantInt::get(Type::getInt32Ty(llvmContext), 0);
        case wabt::Type::I64:
            return ConstantInt::get(Type::getInt64Ty(llvmContext), 0);
        case wabt::Type::F32:
            return llvm::ConstantFP::get(Type::getFloatTy(llvmContext), 0);
        case wabt::Type::F64:
            return llvm::ConstantFP::get(Type::getDoubleTy(llvmContext), 0);
        case wabt::Type::V128:
            return ConstantInt::get(Type::getInt128Ty(llvmContext), 0);
        case wabt::Type::Void:
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot convert type: " << ty.GetName() << std::endl;
            std::abort();
    }
}
}