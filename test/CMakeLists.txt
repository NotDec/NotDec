find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(add_type_recovery_suite test_name manifest_relpath)
	add_test(
		NAME ${test_name}
		COMMAND
			${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/run_type_recovery_suite.py
			--binary
			$<TARGET_FILE:notdec-decompile>
			--manifest
			${CMAKE_CURRENT_SOURCE_DIR}/${manifest_relpath}
			--project-root
			${PROJECT_SOURCE_DIR}
			--workdir
			${CMAKE_CURRENT_BINARY_DIR}/artifacts/${test_name}
	)
endfunction()

add_type_recovery_suite(
	notdec.type_recovery.llvm_ir.tr_level_2
	type-recovery/llvm-ir/manifest.json
)

add_type_recovery_suite(
	notdec.type_recovery.sysy.tr_level_2
	type-recovery/sysy/manifest.json
)

add_type_recovery_suite(
	notdec.type_recovery.howard_o3_split.tr_level_2
	type-recovery/howard-o3-split/manifest.json
)
