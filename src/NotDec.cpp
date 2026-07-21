#include <iostream>
#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

#ifdef NOTDEC_ENABLE_WASM
#include "notdec-wasm2llvm/interface.h"
#endif

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#endif

#include "binarysub/binarysub-primitive-semantics.h"
#include "Passes/PassManager.h"
#include "Utils/Utils.h"

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
    cl::Required, cl::cat(NotdecCat));
static cl::opt<std::string> outputFilename(
    "o",
    cl::desc("Specify output filename. Supported extension: .ll, .bc, .c, .sol"),
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
             cl::init(level_notice), cl::cat(NotdecCat));

#ifdef NOTDEC_ENABLE_WASM
#include "notdec-wasm2llvm/commandlines.def"

static cl::opt<log_level> Wasm2LLVMLogLevel(
    "wasm2llvm-log-level", cl::desc("Log level:"),
    cl::values(clEnumValN(level_emergent, "emergent", "emergent"),
               clEnumValN(level_alert, "alert", "alert"),
               clEnumValN(level_critical, "critical", "critical"),
               clEnumValN(level_error, "error", "error"),
               clEnumValN(level_warning, "warning", "warning"),
               clEnumValN(level_notice, "notice", "notice"),
               clEnumValN(level_info, "info", "info"),
               clEnumValN(level_debug, "debug", "debug")),
    cl::init(level_notice), cl::cat(Wasm2llvmCat));
#endif

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Commandlines.def"
#endif

static cl::opt<int>
    trLevel("tr-level",
                cl::desc("Type recovery level: 0: disable, 1: simple opt, 2: type recovery without breaking stack, 3: full type recovery"),
                cl::init(3), cl::cat(NotdecCat));

static cl::opt<std::string> dumpHTypes(
    "dump-htypes",
    cl::desc("Dump recovered HType results to the specified file."),
    cl::init(""), cl::value_desc("output.htypes"), cl::Optional,
    cl::cat(NotdecCat));

static cl::opt<std::string> mergeEvalDir(
    "merge-eval-dir",
    cl::desc("Write DebugInfo merge-policy evaluation files to this directory."),
    cl::init(""), cl::value_desc("dir"), cl::Optional, cl::cat(NotdecCat));

static cl::opt<bool> mergeStructPtrLoadStore(
    "merge-struct-ptr-load-store",
    cl::desc("Merge load/store target variables when both look like struct "
             "pointers. Enabled by default; pass=false disables it."),
    cl::init(true), cl::cat(NotdecCat));

static cl::list<std::string> primitiveSemanticLatticeFiles(
    "primitive-semantic-lattice",
    cl::desc("Load a primitive semantic lattice family definition from a DOT "
             "file. May be specified multiple times."),
    cl::value_desc("path.dot"), cl::ZeroOrMore, cl::cat(NotdecCat));

static cl::opt<bool> genWorkDir(
    "gen-work-dir",
    cl::desc("Generate intermediate work files in a work directory."),
    cl::init(false), cl::cat(NotdecCat));

static cl::alias genWorkDirShort(
    "g", cl::desc("Alias for --gen-work-dir"), cl::aliasopt(genWorkDir),
    cl::cat(NotdecCat));

static cl::opt<std::string> workDirOverride(
    "work-dir",
    cl::desc("Override the generated work directory path. Requires "
             "--gen-work-dir."),
    cl::init(""), cl::value_desc("path"), cl::Optional, cl::cat(NotdecCat));

static cl::opt<std::string> emitTRInputIR(
    "emit-tr-input-ir",
    cl::desc("Run the pre-type-recovery normalization pipeline, emit the "
             "resulting LLVM IR (.ll/.bc), and exit before type recovery."),
    cl::init(""), cl::value_desc("output.ll"), cl::Optional,
    cl::cat(NotdecCat));

static cl::opt<bool> frozenTRInputIR(
    "frozen-tr-input-ir",
    cl::desc("Treat the input module as a frozen pre-type-recovery IR emitted "
             "by --emit-tr-input-ir. This is the explicit stage-B mode and "
             "requires --tr-level >= 2."),
    cl::init(false), cl::cat(NotdecCat));

// https://llvm.org/docs/ProgrammersManual.html#the-llvm-debug-macro-and-debug-option
// initialize function for the fine-grained debug info with DEBUG_TYPE and the
// -debug-only option
namespace llvm {
void initDebugOptions();
}

namespace {

binarysub::expected<void, binarysub::Error>
configurePrimitiveSemanticRegistry(const notdec::Options &Opts) {
  binarysub::clearGlobalPrimitiveSemanticRegistry();
  auto &Registry = binarysub::globalPrimitiveSemanticRegistry();

  for (const auto &Path : Opts.primitiveSemanticLatticeFiles) {
    auto Family = Registry.registerFamilyFromDotFile(Path);
    if (!Family) {
      return binarysub::make_unexpected(Family.error());
    }
  }

  return {};
}

std::string describeInputSuffix(llvm::StringRef Suffix) {
  return Suffix.empty() ? std::string("<no extension>") : Suffix.str();
}

void printFrozenTRInputWorkflowHint(llvm::StringRef InputPath) {
  llvm::errs() << "Hint: stage B expects frozen .ll/.bc from "
                  "--emit-tr-input-ir.\n"
               << "Example stage A:\n"
               << "  ./build/bin/notdec " << InputPath
               << " --emit-tr-input-ir=/tmp/notdec-tr-input.ll --tr-level=2\n"
               << "Example stage B:\n"
               << "  NOTDEC_EXTRA_CONSTRAINTS=<constraints.json> "
                  "./build/bin/notdec /tmp/notdec-tr-input.ll --tr-level=2 "
                  "--frozen-tr-input-ir -o /tmp/out.ll\n";
}

void configureWorkDirLLVMReports(const notdec::Options &Opts) {
  if (Opts.workDir.empty()) {
    return;
  }

  auto &RegisteredOptions = llvm::cl::getRegisteredOptions();
  auto *TimePasses = RegisteredOptions.lookup("time-passes");
  auto *TimePassesPerRun = RegisteredOptions.lookup("time-passes-per-run");
  auto *InfoOutputFile = RegisteredOptions.lookup("info-output-file");
  if (TimePasses == nullptr || TimePassesPerRun == nullptr ||
      InfoOutputFile == nullptr) {
    return;
  }

  const bool WantsTimingReport =
      TimePasses->getNumOccurrences() > 0 ||
      TimePassesPerRun->getNumOccurrences() > 0;
  if (!WantsTimingReport || InfoOutputFile->getNumOccurrences() > 0) {
    return;
  }

  std::error_code EC = llvm::sys::fs::create_directories(Opts.workDir);
  if (EC) {
    llvm::errs() << "Warning: failed to create work dir for LLVM timing "
                    "report: "
                 << Opts.workDir << ": " << EC.message() << "\n";
    return;
  }

  auto ReportPath = notdec::join(Opts.workDir, "02-time-passes.txt");
  if (InfoOutputFile->addOccurrence(0, "info-output-file", ReportPath)) {
    llvm::errs() << "Warning: failed to route LLVM timing report to "
                 << ReportPath << "\n";
  }
}

[[noreturn]] void printTerminateStackTrace() {
  llvm::errs() << "Fatal: unhandled C++ exception or terminate() called.\n";
  if (auto Ex = std::current_exception()) {
    try {
      std::rethrow_exception(Ex);
    } catch (const std::exception &E) {
      llvm::errs() << "Exception: " << E.what() << "\n";
    } catch (...) {
      llvm::errs() << "Exception: non-std exception\n";
    }
  }
  llvm::sys::PrintStackTrace(llvm::errs());
  std::abort();
}

} // namespace

int main(int argc, char *argv[]) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  std::set_terminate(printTerminateStackTrace);
  // initDebugOptions();
  // parse cmdline
  cl::ParseCommandLineOptions(argc, argv);
  const char *ExtraConstraintsFile = std::getenv("NOTDEC_EXTRA_CONSTRAINTS");
  std::string insuffix = getSuffix(inputFilename);
  std::string InputSuffixDesc = describeInputSuffix(insuffix);
  if (!workDirOverride.empty() && !genWorkDir) {
    llvm::errs() << "Error: --work-dir requires --gen-work-dir.\n";
    return 1;
  }
  if (!emitTRInputIR.empty() && !outputFilename.empty()) {
    llvm::errs() << "Error: --emit-tr-input-ir cannot be combined with -o.\n";
    return 1;
  }
  if (!emitTRInputIR.empty() && !dumpHTypes.empty()) {
    llvm::errs() << "Error: --emit-tr-input-ir cannot be combined with "
                    "--dump-htypes.\n";
    return 1;
  }
  if (!emitTRInputIR.empty() && !mergeEvalDir.empty()) {
    llvm::errs() << "Error: --emit-tr-input-ir cannot be combined with "
                    "--merge-eval-dir.\n";
    return 1;
  }
  if (frozenTRInputIR && !emitTRInputIR.empty()) {
    llvm::errs() << "Error: --frozen-tr-input-ir cannot be combined with "
                    "--emit-tr-input-ir.\n";
    return 1;
  }
  if (ExtraConstraintsFile != nullptr && !emitTRInputIR.empty()) {
    llvm::errs() << "Error: NOTDEC_EXTRA_CONSTRAINTS cannot be combined with "
                    "--emit-tr-input-ir.\n"
                 << "Reason: --emit-tr-input-ir is stage A, while "
                    "NOTDEC_EXTRA_CONSTRAINTS is stage B.\n";
    printFrozenTRInputWorkflowHint(inputFilename);
    return 1;
  }
  if (ExtraConstraintsFile != nullptr &&
      insuffix != ".ll" && insuffix != ".bc") {
    llvm::errs() << "Error: NOTDEC_EXTRA_CONSTRAINTS only supports frozen "
                    ".ll/.bc stage-B inputs, but current input "
                 << inputFilename << " has suffix " << InputSuffixDesc
                 << ".\n"
                 << "Reason: extra-constraint selectors and ir_anchor are "
                    "interpreted against frozen pre-type-recovery LLVM IR, "
                    "not against raw frontend input.\n";
    printFrozenTRInputWorkflowHint(inputFilename);
    return 1;
  }
  if (ExtraConstraintsFile != nullptr && !frozenTRInputIR) {
    llvm::errs() << "Error: NOTDEC_EXTRA_CONSTRAINTS requires "
                    "--frozen-tr-input-ir even when the input is "
                 << InputSuffixDesc << ".\n"
                 << "Reason: the flag makes stage-B semantics explicit and "
                    "ensures the pre-type-recovery normalization pipeline is "
                    "not rerun.\n";
    printFrozenTRInputWorkflowHint(inputFilename);
    return 1;
  }
  notdec::Options opts{
      .trLevel = trLevel,
      .log_level = LogLevel,
      .primitiveSemanticLatticeFiles =
          std::vector<std::string>(primitiveSemanticLatticeFiles.begin(),
                                   primitiveSemanticLatticeFiles.end()),
  };
  opts.emitTRInputIR = emitTRInputIR;
  opts.mergeEvalDir = mergeEvalDir;
  opts.mergeStructPtrLoadStore = mergeStructPtrLoadStore;
  opts.frozenTRInputIR = frozenTRInputIR;
  if (genWorkDir) {
    opts.workDir = workDirOverride.empty()
                       ? notdec::getDefaultWorkDir(inputFilename)
                       : workDirOverride;
  }

  if (auto RegistrySetup = configurePrimitiveSemanticRegistry(opts);
      !RegistrySetup) {
    llvm::errs() << "Error: failed to configure primitive semantic lattice: "
                 << RegistrySetup.error().msg << "\n";
    return 1;
  }

  notdec::setWorkDir(opts.workDir);
  configureWorkDirLLVMReports(opts);

  if (frozenTRInputIR && insuffix != ".ll" && insuffix != ".bc") {
    llvm::errs() << "Error: --frozen-tr-input-ir requires a .ll or .bc input, "
                    "but current input "
                 << inputFilename << " has suffix " << InputSuffixDesc
                 << ".\n"
                 << "Reason: stage B runs directly on frozen LLVM IR rather "
                    "than on raw frontend input.\n";
    printFrozenTRInputWorkflowHint(inputFilename);
    return 1;
  }
  if (frozenTRInputIR && trLevel < 2) {
    llvm::errs() << "Error: --frozen-tr-input-ir requires --tr-level >= 2, "
                    "but current value is "
                 << trLevel << ".\n"
                 << "Reason: stage B means running type recovery on frozen "
                    "LLVM IR, so levels 0/1 are not valid in this mode.\n";
    printFrozenTRInputWorkflowHint(inputFilename);
    return 1;
  }
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
    notdec::frontend::wasm::Options WasmOpts;
    // override frontend config if decompilation is enabled
    if (trLevel >= 2) {
      WasmOpts = notdec::frontend::wasm::Options{
          .GenIntToPtr = true,
          .SplitMem = true,
          .LogLevel = Wasm2LLVMLogLevel,
      };
    } else {
      WasmOpts = getWasmOptions(Wasm2LLVMLogLevel);
    }
    std::cout << "Loading Wasm: " << inputFilename << std::endl;
    notdec::frontend::parse_wasm(Ctx.context, Ctx.getModule(), WasmOpts,
                                 inputFilename);
  } else if (insuffix == ".wat") {
    notdec::frontend::wasm::Options WasmOpts;
    // override frontend config if decompilation is enabled
    if (trLevel >= 2) {
      WasmOpts = notdec::frontend::wasm::Options{
          .GenIntToPtr = true,
          .SplitMem = true,
          .LogLevel = Wasm2LLVMLogLevel,
      };
    } else {
      WasmOpts = getWasmOptions(Wasm2LLVMLogLevel);
    }
    std::cout << "Loading Wat: " << inputFilename << std::endl;
    notdec::frontend::parse_wat(Ctx.context, Ctx.getModule(), WasmOpts,
                                inputFilename);
  }
#endif
  else {
    std::cout << "unknown extension " << insuffix << " for input file. exiting."
              << std::endl;
    return 0;
  }

  auto &M = Ctx.getModule();
  auto llvm2cOpts = getLLVM2COptions();
  llvm2cOpts.workDir = Ctx.opt.workDir;
  notdec::passes::DecompileConfig conf(M, outputFilename, dumpHTypes, Ctx.opt,
                                       llvm2cOpts);
  conf.build_passes(trLevel);
  conf.run_passes();

  if (!emitTRInputIR.empty()) {
    conf.emit_tr_input_ir();
    return 0;
  }

  std::string outsuffix = getSuffix(outputFilename);
  if (outsuffix == ".c" || outsuffix == ".sol") {
    // do nothing, because we will add backend output pass
  } else if (outsuffix == ".ll") {
    std::error_code EC;
    llvm::raw_fd_ostream os(outputFilename, EC);
    if (EC) {
      std::cerr << "Cannot open output file." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    M.print(os, nullptr);
    std::cout << "IR dumped to " << outputFilename << std::endl;
  } else if (outsuffix == ".bc") {
    std::error_code EC;
    llvm::raw_fd_ostream os(outputFilename, EC);
    if (EC) {
      std::cerr << "Cannot open output file." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    llvm::WriteBitcodeToFile(M, os);
    std::cout << "Bitcode dumped to " << outputFilename << std::endl;
  } else {
    std::cout << "Error: Unknown suffix to output " << outputFilename
              << std::endl;
    std::abort();
  }

  notdec::frontend::free_buffer();
  return 0;
}
