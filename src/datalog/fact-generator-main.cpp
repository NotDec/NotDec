#include <iostream>

#include "llvm/Support/CommandLine.h"
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>

#include "datalog/fact-generator.h"

using namespace llvm;

static llvm::cl::opt<std::string> inputFilename("i",llvm::cl::desc("input wasm/wat file"), llvm::cl::value_desc("input.ll"), llvm::cl::Required);
static llvm::cl::opt<std::string> outputDirname("o",llvm::cl::desc("Specify output dirname"), llvm::cl::value_desc("out"), llvm::cl::Optional);

static std::string getSuffix(std::string fname) {
    std::size_t ind = fname.find_last_of('.');
    if (ind != std::string::npos) {
        return fname.substr(ind);
    }
    return std::string();
}

int main(int argc, char * argv[]) {
    llvm::cl::ParseCommandLineOptions(argc,argv);
    std::cout << "input file: " << inputFilename << std::endl;
    std::cout << "output dir: " << outputDirname << std::endl;
    std::string insuffix = getSuffix(inputFilename);
    if (insuffix.size() == 0) {
        std::cout << "no extension for input file. exiting." << std::endl;
        return 0;
    }
    // load LLVM module
    if (insuffix == ".ll" || insuffix == ".bc") {
        LLVMContext Context;
        SMDiagnostic Error;
        std::unique_ptr<Module> Mod = parseIRFile(inputFilename, Error, Context);
        if (!Mod) {
            std::cout << "failed to load LLVM module." << std::endl;
            return 0;
        }
        notdec::datalog::FactGenerator::generate(*Mod, outputDirname.c_str());
    } else {
        std::cout << "unsupported input file extension." << std::endl;
    }
    return 0;
}
