#include <iostream>
#include <string>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#ifdef NOTDEC_ENABLE_WASM
#include "notdec-wasm2llvm/interface.h"
#endif
#include "notdec-llvm2c/structural-analysis.h"

#include "optimizers/opt-manager.h"
#include "utils.h"

using namespace llvm;

static cl::OptionCategory NotdecCat("Notdec decompiler middle end options",
                                    "These control the notdec middle end.");

// https://llvm.org/docs/CommandLine.html
static cl::opt<std::string> inputFilename(
    cl::Positional, cl::desc("<input file>"),
    cl::value_desc("input file for decompilation. Supported files: .ll, .bc"
#ifdef NOTDEC_ENABLE_WASM
                   ", .wasm, .wat"
#endif
                   ),
    cl::Required);
static cl::opt<std::string> outputFilename(
    "o", cl::desc("Specify output filename. Supported extension: .ll, .bc, .c"),
    cl::value_desc("output.ll"), cl::Optional, cl::cat(NotdecCat));

static cl::opt<log_level>
    LogLevel("log-level", cl::desc("Log level:"),
             cl::values(clEnumValN(level_emergent, "emergent", "emergent"),
                        clEnumValN(level_alert, "alert", "alert"),
                        clEnumValN(level_critical, "critical", "critical"),
                        clEnumValN(level_error, "error", "error"),
                        clEnumValN(level_warning, "warning", "warning"),
                        clEnumValN(level_notice, "notice", "notice"),
                        clEnumValN(level_info, "info", "info"),
                        clEnumValN(level_debug, "debug", "debug")),
             cl::init(level_notice));

#ifdef NOTDEC_ENABLE_WASM
#include "notdec-wasm2llvm/commandlines.def"
#endif

static cl::opt<std::string> stackRec(
    "stack-recovery-algo",
    cl::desc(
        "stack recovery algorithm to use: retdec or notdec. default: notdec"),
    cl::init("notdec"), cl::value_desc("sta-algo"), cl::Optional,
    cl::cat(NotdecCat));
static cl::opt<bool> onlyOpt(
    "only-opt",
    cl::desc(
        "Disable decompilation passes. Only perform some optimization passes."),
    cl::init(false), cl::cat(NotdecCat));

static cl::opt<bool>
    disableAllPasses("disable-all-pass",
                     cl::desc("Disable all passes in the middle end"),
                     cl::init(false), cl::cat(NotdecCat));

std::string getSuffix(std::string fname) {
  std::size_t ind = fname.find_last_of('.');
  if (ind != std::string::npos) {
    return fname.substr(ind);
  }
  return std::string();
}

// https://llvm.org/docs/ProgrammersManual.html#the-llvm-debug-macro-and-debug-option
// initialize function for the fine-grained debug info with DEBUG_TYPE and the
// -debug-only option
namespace llvm {
void initDebugOptions();
}

int main(int argc, char *argv[]) {
  initDebugOptions();
  // parse cmdline
  cl::ParseCommandLineOptions(argc, argv);
  notdec::Options opts{
      .onlyOptimize = onlyOpt,
      .disableAllPasses = disableAllPasses,
      .stackRec = stackRec,
      .log_level = LogLevel,
  };

  // if log level is debug, also enable debug flag
  if (LogLevel == level_debug) {
    DebugFlag = true;
  }

  std::string insuffix = getSuffix(inputFilename);
  notdec::DecompilerContext Ctx(inputFilename, opts);
  if (insuffix.size() == 0) {
    std::cout << "no extension for input file. exiting." << std::endl;
    return 0;
  } else if (insuffix == ".ll" || insuffix == ".bc") {
    std::cout << "Loading LLVM IR: " << inputFilename << std::endl;
    SMDiagnostic Err;
    Ctx.setModule(parseIRFile(inputFilename, Err, Ctx.context));
    // TODO: enable optimization?
    if (!Ctx.hasModule()) {
      Err.print("IR parsing failed: ", errs());
      return 0;
    }
  }
#ifdef NOTDEC_ENABLE_WASM
  else if (insuffix == ".wasm") {
    std::cout << "Loading Wasm: " << inputFilename << std::endl;
    notdec::frontend::parse_wasm(Ctx.context, Ctx.getModule(), getWasmOptions(),
                                 inputFilename);
  } else if (insuffix == ".wat") {
    std::cout << "Loading Wat: " << inputFilename << std::endl;
    notdec::frontend::parse_wat(Ctx.context, Ctx.getModule(), getWasmOptions(),
                                inputFilename);
  }
#endif
  else {
    std::cout << "unknown extension " << insuffix << " for input file. exiting."
              << std::endl;
    return 0;
  }

  // run passes and dump IR
  if (!opts.disableAllPasses) {
    notdec::optimizers::DecompileConfig conf(Ctx.getModule(), Ctx.opt);
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
    Ctx.getModule().print(os, nullptr);
    std::cout << "IR dumped to " << outputFilename << std::endl;
  } else if (outsuffix == ".bc") {
    std::error_code EC;
    llvm::raw_fd_ostream os(outputFilename, EC);
    if (EC) {
      std::cerr << "Cannot open output file." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    llvm::WriteBitcodeToFile(Ctx.getModule(), os);
    std::cout << "Bitcode dumped to " << outputFilename << std::endl;
  } else if (outsuffix == ".c") {
    std::error_code EC;
    llvm::raw_fd_ostream os(outputFilename, EC);
    if (EC) {
      std::cerr << "Cannot open output file." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    notdec::llvm2c::decompileModule(Ctx.getModule(), os);
    std::cout << "Decompile result: " << outputFilename << std::endl;
  }

  return 0;
}
