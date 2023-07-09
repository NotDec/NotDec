#include <iostream>
#include <string>

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#ifdef NOTDEC_ENABLE_WASM
#include "frontend/wasm/parser.h"
#include "optimizers/opt-manager.h"
#endif

//https://llvm.org/docs/CommandLine.html
static cl::opt<std::string> inputFilename("i",cl::desc("input wasm/wat file"), cl::value_desc("wasm/wat"),cl::Required);
static cl::opt<std::string> outputFilename("o",cl::desc("Specify output filename"), cl::value_desc("output.ll"),cl::Optional);
static cl::opt<bool> recompile ("recompile", cl::desc("Enable recompile mode, only perform lifting and optimization, not performing further decompilation like stack recovery."),cl::init(false));
static cl::opt<bool> compatMode ("compat-mode", cl::desc("Make IR more compatible, e.g., rename main function to main."),cl::init(true));
static cl::opt<bool> disablePass ("disable-pass", cl::desc("Disable all passes"),cl::init(false));
static cl::opt<bool> expandMem ("expand-mem", cl::desc("for debug purpose, not expand mem to real size"),cl::init(true));

cl::opt<log_level> logLevel("log-level",cl::desc("Choose log level:"),
    cl::values(
    clEnumVal(level_emergent, "emergent"),
    clEnumVal(level_alert, "alert"),
    clEnumVal(level_critical, "critical"),
    clEnumVal(level_error, "error"),
    clEnumVal(level_warning, "warning"),
    clEnumVal(level_notice, "notice"),
    clEnumVal(level_info, "info"),
    clEnumVal(level_debug, "debug")),cl::init(level_notice));

std::string getSuffix(std::string fname) {
    std::size_t ind = fname.find_last_of('.');
    if (ind != std::string::npos) {
        return fname.substr(ind);
    }
    return std::string();
}

int main(int argc, char * argv[]) {
    
    // parse cmdline
    cl::ParseCommandLineOptions(argc,argv);
    notdec::frontend::options opts;
    opts.recompile = recompile;
    opts.compat_mode = compatMode;
    opts.log_level = logLevel;
    opts.expandMem = expandMem;

    std::string insuffix = getSuffix(inputFilename);
    notdec::frontend::BaseContext ctx(inputFilename, opts);
    if (insuffix.size() == 0) {
        std::cout << "no extension for input file. exiting." << std::endl;
        return 0;
    }
#ifdef NOTDEC_ENABLE_WASM
    else if (insuffix == ".wasm") {
        std::cout << "using wasm frontend." << std::endl;
        notdec::frontend::wasm::parse_wasm(ctx, inputFilename);
        // TODO
    } else if (insuffix == ".wat") {
        std::cout << "using wat frontend." << std::endl;
        notdec::frontend::wasm::parse_wat(ctx, inputFilename);
        // TODO
    }
#endif
    else {
        std::cout << "unknown extension " << insuffix << " for input file. exiting." << std::endl;
        return 0;
    }

    //run passes and dump IR
    if (!disablePass) {
        notdec::frontend::optimizers::DecompileConfig conf(ctx.mod, ctx.opt);
        conf.run_passes();
    }
    std::string outsuffix = getSuffix(outputFilename);
    if (outsuffix == ".ll") {
        std::error_code EC;
        llvm::raw_fd_ostream os(outputFilename, EC);
        if (EC) {
            std::cerr << "Cannot open output file." << std::endl;
            std::cerr << EC.message() << std::endl;
            std::abort();
        }
        ctx.mod.print(os, nullptr);
        std::cout << "IR dumped to " << outputFilename << std::endl;
    }
    
    return 0;
}