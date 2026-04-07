find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
	NAME notdec.type_recovery.llvm_ir.tr_level_2
	COMMAND
		${Python3_EXECUTABLE}
		${CMAKE_CURRENT_SOURCE_DIR}/tools/run_type_recovery_suite.py
		--binary
		$<TARGET_FILE:notdec-decompile>
		--manifest
		${CMAKE_CURRENT_SOURCE_DIR}/decompile/llvm-ir/type-recovery-tr-level-2.json
		--project-root
		${PROJECT_SOURCE_DIR}
		--workdir
		${CMAKE_CURRENT_BINARY_DIR}/artifacts/notdec.type_recovery.llvm_ir.tr_level_2
)
