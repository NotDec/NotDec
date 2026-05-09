
add_library(notdec STATIC
	# source files that are common to all frontend
	Passes/PassManager.cpp
	Passes/StackPointerFinder.cpp
	Passes/StackAlloca.cpp
	Passes/MemOpMatcher.cpp
	Passes/ReorderBasicblock.cpp
	Passes/AllocAnnotator.cpp
	TypeRecovery/LowTy.cpp
	TypeRecovery/mlsub/PNDiff.cpp
	TypeRecovery/mlsub/PAPath.cpp
	TypeRecovery/mlsub/PointerAnalysis.cpp
	# TypeRecovery/mlsub/MLsubGraph.cpp
	TypeRecovery/mlsub/HTypeDebug.cpp
	TypeRecovery/mlsub/HTypeNormalize.cpp
	TypeRecovery/mlsub/MLsubGenerator.cpp
	TypeRecovery/mlsub/Schema.cpp
	TypeRecovery/mlsub/TypeBuilder.cpp
	Utils/Utils.cpp
)

# 主项目源码里有一部分类型恢复和 pass 逻辑直接复用了 llvm2c 的头文件类型，
# 即使暂时不构建 llvm2c 库，编这些源文件时也仍然需要头文件可见。
target_include_directories(notdec
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
target_compile_definitions(notdec
	PRIVATE
	NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH="${NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD}"
)

# include直接在外部设置了src目录。

if (NOTDEC_ENABLE_WASM)
	target_link_libraries(notdec
		PUBLIC
		notdec-wasm2llvm
	)
endif()

if (NOTDEC_ENABLE_LLVM2C)
	# target_sources(notdec PRIVATE
	# 	backend/main.cpp
	# )
	target_link_libraries(notdec
		PUBLIC
		notdec-llvm2c
	)
endif()

target_link_libraries(notdec
	PUBLIC
	notdec_llvm_deps
	binarysub_lib
)

set_target_properties(notdec
	PROPERTIES
		OUTPUT_NAME "notdec"
)

# 添加可执行文件

add_executable(notdec-decompile 
	NotDec.cpp
)

# target_compile_features(bin2pat PUBLIC cxx_std_17)

target_link_libraries(notdec-decompile 
	notdec
)

set_target_properties(notdec-decompile
	PROPERTIES
		OUTPUT_NAME "notdec"
)

install(TARGETS notdec-decompile DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(NOTDEC_ENABLE_RETDEC_LLVMIR2HLL)
	# # remove -fno-rtti
	# string(REPLACE " -fno-rtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	# add_subdirectory(backend/retdec-llvmir2hll)
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
	# # enable retdec interface in notdec
	# target_compile_definitions(notdec PUBLIC ENABLE_RETDEC_LLVMIR2HLL)
endif()
