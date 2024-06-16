#include "backend/structural-analysis.h"
#include <iostream>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Debug.h>
#include <string>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#ifdef NOTDEC_ENABLE_WASM
#include "frontend/wasm/parser.h"
#include "optimizers/opt-manager.h"
#endif

using namespace llvm;

// https://llvm.org/docs/CommandLine.html
static cl::opt<std::string> inputFilename("i", cl::desc("input wasm/wat file"),
                                          cl::value_desc("wasm/wat"),
                                          cl::Required);
static cl::opt<std::string> outputFilename("o",
                                           cl::desc("Specify output filename"),
                                           cl::value_desc("output.ll"),
                                           cl::Optional);
static cl::opt<std::string> stackRec(
    "stack-recovery-algo",
    cl::desc(
        "stack recovery algorithm to use: retdec or notdec. default: notdec"),
    cl::init("notdec"), cl::value_desc("sta-algo"), cl::Optional);
static cl::opt<bool> recompile(
    "recompile",
    cl::desc("Enable recompile mode, only perform lifting and optimization, "
             "not performing further decompilation like stack recovery."),
    cl::init(false));
static cl::opt<bool> compatMode(
    "compat-mode",
    cl::desc("Make IR more compatible, e.g., rename main function to main."),
    cl::init(true));
static cl::opt<bool>
    disablePass("disable-pass",
                cl::desc("Disable all passes in the middle end"),
                cl::init(false));
static cl::opt<bool> expandMem(
    "expand-mem",
    cl::desc(
        "(for recompile mode) not expand mem to real size for debug purpose."),
    cl::init(true));

cl::opt<log_level>
    logLevel("log-level", cl::desc("Log level:"),
             cl::values(clEnumValN(level_emergent, "emergent", "emergent"),
                        clEnumValN(level_alert, "alert", "alert"),
                        clEnumValN(level_critical, "critical", "critical"),
                        clEnumValN(level_error, "error", "error"),
                        clEnumValN(level_warning, "warning", "warning"),
                        clEnumValN(level_notice, "notice", "notice"),
                        clEnumValN(level_info, "info", "info"),
                        clEnumValN(level_debug, "debug", "debug")),
             cl::init(level_notice));

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
  notdec::frontend::options opts;
  opts.recompile = recompile;
  opts.compat_mode = compatMode;
  opts.log_level = logLevel;
  opts.expandMem = expandMem;
  opts.stackRec = stackRec;

  // if log level is debug, also enable debug flag
  if (logLevel == level_debug) {
    DebugFlag = true;
  }

  std::string insuffix = getSuffix(inputFilename);
  notdec::frontend::BaseContext ctx(inputFilename, opts);
  if (insuffix.size() == 0) {
    std::cout << "no extension for input file. exiting." << std::endl;
    return 0;
  } else if (insuffix == ".ll" || insuffix == ".bc") {
    std::cout << "Loading LLVM IR: " << inputFilename << std::endl;
    SMDiagnostic Err;
    ctx.setModule(parseIRFile(inputFilename, Err, ctx.context));
    // TODO: enable optimization?
    if (!ctx.hasModule()) {
      Err.print("IR parsing failed: ", errs());
      return 0;
    }
    disablePass = true;
  }
#ifdef NOTDEC_ENABLE_WASM
  else if (insuffix == ".wasm") {
    ctx.opt.from_wasm = true;
    std::cout << "using wasm frontend." << std::endl;
    notdec::frontend::wasm::parse_wasm(ctx, inputFilename);
  } else if (insuffix == ".wat") {
    ctx.opt.from_wasm = true;
    std::cout << "using wat frontend." << std::endl;
    notdec::frontend::wasm::parse_wat(ctx, inputFilename);
  }
#endif
  else {
    std::cout << "unknown extension " << insuffix << " for input file. exiting."
              << std::endl;
    return 0;
  }

  // run passes and dump IR
  if (!disablePass) {
    notdec::optimizers::DecompileConfig conf(ctx.getModule(), ctx.opt);
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
    ctx.getModule().print(os, nullptr);
    std::cout << "IR dumped to " << outputFilename << std::endl;
  } else if (outsuffix == ".c") {
    std::error_code EC;
    llvm::raw_fd_ostream os(outputFilename, EC);
    if (EC) {
      std::cerr << "Cannot open output file." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    notdec::backend::decompileModule(ctx.getModule(), os);
    std::cout << "Decompile result: " << outputFilename << std::endl;
  }

  return 0;
}
