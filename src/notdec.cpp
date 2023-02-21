#include <iostream>
#include <algorithm>

#include "llvm/Support/raw_ostream.h"

#ifdef NOTDEC_ENABLE_WASM
#include "frontend/wasm/parser.h"
#endif

// https://stackoverflow.com/questions/865668/parsing-command-line-arguments-in-c
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

std::string getSuffix(std::string fname) {
    std::size_t ind = fname.find_last_of('.');
    if (ind != std::string::npos) {
        return fname.substr(ind);
    }
    return std::string();
}

int main(int argc, char * argv[]) {
    notdec::frontend::options opts;

    // parse cmdline
    if(cmdOptionExists(argv, argv+argc, "-h") || cmdOptionExists(argv, argv+argc, "--help")) {
usage:
        std::cout << "Usage: " << argv[0] << "-i wasm_file -o llvm_ir_file" << std::endl;
        return 0;
    }

    char * outfilename = getCmdOption(argv, argv + argc, "-o");
    char * infilename = getCmdOption(argv, argv + argc, "-i");
    if (!(outfilename && infilename))
    {
        goto usage;
    }

    if (cmdOptionExists(argv, argv+argc, "--recompile")) {
        opts.recompile = true;
    }

    if (cmdOptionExists(argv, argv+argc, "--test-mode")) {
        opts.test_mode = true;
    }
    
    std::string insuffix = getSuffix(infilename);
    notdec::frontend::BaseContext ctx(infilename, opts);
    if (insuffix.size() == 0) {
        std::cout << "no extension for input file. exiting." << std::endl;
        return 0;
    }
#ifdef NOTDEC_ENABLE_WASM
    else if (insuffix == ".wasm") {
        std::cout << "using wasm frontend." << std::endl;
        notdec::frontend::wasm::parse_wasm(ctx, infilename);
        // TODO
    } else if (insuffix == ".wat") {
        std::cout << "using wat frontend." << std::endl;
        notdec::frontend::wasm::parse_wat(ctx, infilename);
        // TODO
    }
#endif
    else {
        std::cout << "unknown extension " << insuffix << " for input file. exiting." << std::endl;
        return 0;
    }

    std::string outsuffix = getSuffix(outfilename);
    if (outsuffix == ".ll") {
        std::error_code EC;
        llvm::raw_fd_ostream os(outfilename, EC);
        if (EC) {
            std::cerr << "Cannot open output file." << std::endl;
            std::cerr << EC.message() << std::endl;
            std::abort();
        }
        ctx.mod.print(os, nullptr);
        std::cout << "IR dumped to " << outfilename << std::endl;
    }
    
    return 0;
}