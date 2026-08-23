
add_library(notdec-core STATIC
	# source files that are common to all frontend
	Passes/PassManager.cpp
	Passes/StackPointerFinder.cpp
	Passes/StackAlloca.cpp
	Passes/MemOpMatcher.cpp
	Passes/ReorderBasicblock.cpp
	Passes/AllocAnnotator.cpp
	Passes/DangerousTypePatternScan.cpp
	Passes/evm/MemoryBufferAnalysis.cpp
	Passes/evm/SolidityPatterns.cpp
	Passes/evm/solidity-patterns/AbiReturnPass.cpp
	Passes/evm/solidity-patterns/AbiDecoderHelperRenamePass.cpp
	Passes/evm/solidity-patterns/CheckedBoundsMatchers.cpp
	Passes/evm/solidity-patterns/CheckedBoundsPass.cpp
	Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp
	Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp
	Passes/evm/solidity-patterns/EventLogPass.cpp
	Passes/evm/solidity-patterns/HTypeBufferView.cpp
	Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp
	Passes/evm/solidity-patterns/PayabilityGuardMatchers.cpp
	Passes/evm/solidity-patterns/PayabilityGuardPass.cpp
	Passes/evm/solidity-patterns/PublicEntryConstants.cpp
	Passes/evm/solidity-patterns/SelectorEntryMatchers.cpp
	Passes/evm/solidity-patterns/SelectorEntryOutliningPass.cpp
	Passes/evm/solidity-patterns/SolidityRevertPass.cpp
	Passes/evm/solidity-patterns/StorageAccessMatchers.cpp
	TypeRecovery/LowTy.cpp
	# TypeRecovery/mlsub/MLsubGraph.cpp
	TypeRecovery/mlsub/HTypeDebug.cpp
	TypeRecovery/mlsub/HTypeNormalize.cpp
	TypeRecovery/mlsub/AnonymousPolyBoundaryAnalysis.cpp
	TypeRecovery/mlsub/MergePolicyEval.cpp
	TypeRecovery/mlsub/MLsubGenerator.cpp
	TypeRecovery/mlsub/PostConstraintCheckpoint.cpp
	TypeRecovery/mlsub/Schema.cpp
	Utils/Utils.cpp
)

# 主项目源码里有一部分类型恢复和 pass 逻辑直接复用了 llvm2c 的头文件类型，
# 即使暂时不构建 llvm2c 库，编这些源文件时也仍然需要头文件可见。
target_include_directories(notdec-core
	PUBLIC
	${PROJECT_SOURCE_DIR}/external/NotDec-llvm2c/include
)

set(NOTDEC_MLSUB_BUILTIN_SUMMARIES_SOURCE
	"${PROJECT_SOURCE_DIR}/resources/mlsub_builtin_summaries.json"
)
set(NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD_DIR
	"${PROJECT_BINARY_DIR}/share/notdec"
)
set(NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD
	"${NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD_DIR}/mlsub_builtin_summaries.json"
)
file(MAKE_DIRECTORY "${NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD_DIR}")
configure_file(
	"${NOTDEC_MLSUB_BUILTIN_SUMMARIES_SOURCE}"
	"${NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD}"
	COPYONLY
)
target_compile_definitions(notdec-core
	PRIVATE
	NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH="${NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD}"
)

# include直接在外部设置了src目录。

if (NOTDEC_ENABLE_WASM)
	target_link_libraries(notdec-core
		PUBLIC
		notdec-wasm2llvm
	)
endif()

if (NOTDEC_ENABLE_LLVM2C)
	# target_sources(notdec-core PRIVATE
	# 	backend/main.cpp
	# )
	target_link_libraries(notdec-core
		PUBLIC
		notdec-llvm2c
		notdec-backend-solidity
	)
endif()

target_link_libraries(notdec-core
	PUBLIC
	notdec_llvm_deps
	binarysub_lib
)

# 添加可执行文件

add_executable(notdec
	NotDec.cpp
)

# target_compile_features(bin2pat PUBLIC cxx_std_17)

target_link_libraries(notdec
	notdec-core
)

set_target_properties(notdec
	PROPERTIES
		OUTPUT_NAME "notdec"
)

install(TARGETS notdec DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(NOTDEC_ENABLE_RETDEC_LLVMIR2HLL)
	# # remove -fno-rtti
	# string(REPLACE " -fno-rtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	# add_subdirectory(backend/retdec-llvmir2hll)
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
	# # enable retdec interface in notdec-core
	# target_compile_definitions(notdec-core PUBLIC ENABLE_RETDEC_LLVMIR2HLL)
endif()
