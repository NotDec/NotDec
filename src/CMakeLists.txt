add_subdirectory(datalog)

add_library(notdec STATIC
	# source files that are common to all frontend
	Passes/PassManager.cpp
	Passes/PointerTypeRecovery.cpp
	Passes/StackPointerFinder.cpp
	Passes/StackAlloca.cpp
	Passes/StackBreaker.cpp
	Passes/MemOpMatcher.cpp
	Passes/ConstraintGenerator.cpp
	Passes/ReorderBasicblock.cpp
	Passes/DSROA.cpp
	Passes/AllocAnnotator.cpp
	Passes/retdec-stack/retdec-abi.cpp
	Passes/retdec-stack/retdec-ir-modifier.cpp
	Passes/retdec-stack/retdec-reaching-definition.cpp
	Passes/retdec-stack/retdec-stack.cpp
	Passes/retdec-stack/retdec-stack-pointer-op-remove.cpp
	Passes/retdec-stack/retdec-symbolic-tree.cpp
	Passes/retdec-stack/retdec-utils.cpp
	TypeRecovery/Lattice.cpp
	TypeRecovery/DotSummaryParser.cpp
	TypeRecovery/Parser.cpp
	TypeRecovery/ConstraintGraph.cpp
	TypeRecovery/Schema.cpp
	TypeRecovery/RExp.cpp
	TypeRecovery/PointerNumberIdentification.cpp
	TypeRecovery/NFAMinimize.cpp
	TypeRecovery/Sketch.cpp
	TypeRecovery/LowTy.cpp
	TypeRecovery/SketchToCTypeBuilder.cpp
	Utils/Utils.cpp
)

# include直接在外部设置了src目录。

target_link_libraries(notdec
	PUBLIC
	LLVMCore LLVMIRReader LLVMSupport LLVMPasses
	clangAST clangTooling
	notdec-datalog
	${Boost_LIBRARIES}
)

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

add_dependencies(notdec souffle_cpp)
set_property(SOURCE ${CMAKE_BINARY_DIR}/datalog/pointer_main.cpp PROPERTY GENERATED 1)

# TODO
target_sources(notdec PRIVATE
	${CMAKE_BINARY_DIR}/datalog/pointer_main.cpp)
set_source_files_properties(${CMAKE_BINARY_DIR}/datalog/pointer_main.cpp PROPERTIES COMPILE_FLAGS "-frtti -w -D__EMBEDDED_SOUFFLE__")
