add_subdirectory(datalog)

add_library(notdec STATIC
	# source files that are common to all frontend
	optimizers/opt-manager.cpp
	optimizers/pointer-type-recovery.cpp
	optimizers/stack-pointer-finder.cpp
	optimizers/stack-alloca.cpp
	optimizers/retypd-generate.cpp
	optimizers/retdec-stack/retdec-abi.cpp
	optimizers/retdec-stack/retdec-ir-modifier.cpp
	optimizers/retdec-stack/retdec-reaching-definition.cpp
	optimizers/retdec-stack/retdec-stack.cpp
	optimizers/retdec-stack/retdec-stack-pointer-op-remove.cpp
	optimizers/retdec-stack/retdec-symbolic-tree.cpp
	optimizers/retdec-stack/retdec-utils.cpp
	backend/structural-analysis.cpp
	backend/phoenix.cpp
	backend/CFG.cpp
	backend/goto.cpp
	backend/utils.cpp
)

# include直接在外部设置了src目录。

target_link_libraries(notdec
	PUBLIC
	LLVMCore LLVMIRReader LLVMSupport LLVMPasses
  clangAST clangTooling
	notdec-datalog
)

if (NOTDEC_ENABLE_WASM)
	target_sources(notdec PRIVATE
		frontend/wasm/parser.cpp
		frontend/wasm/parser-block.cpp
		frontend/wasm/parser-instruction.cpp
	)

	target_link_libraries(notdec
		PUBLIC
		wabt::wabt
	)
	# add_dependencies(notdec wabt)
endif ()



set_target_properties(notdec
	PROPERTIES
		OUTPUT_NAME "notdec"
)

# 添加可执行文件

add_executable(notdec-decompile 
	notdec.cpp
	utils.cpp
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
	# remove -fno-rtti
	string(REPLACE " -fno-rtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_subdirectory(backend/retdec-llvmir2hll)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
	# enable retdec interface in notdec
	target_compile_definitions(notdec PUBLIC ENABLE_RETDEC_LLVMIR2HLL)
endif()


add_dependencies(notdec souffle_cpp)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/datalog/rules/pointer_main.cpp PROPERTY GENERATED 1)

target_sources(notdec PRIVATE
        datalog/rules/pointer_main.cpp)
set_source_files_properties(datalog/rules/pointer_main.cpp PROPERTIES COMPILE_FLAGS "-frtti -w -D__EMBEDDED_SOUFFLE__")
