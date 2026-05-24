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

function(add_lifting_suite test_name manifest_relpath)
	add_test(
		NAME ${test_name}
		COMMAND
			${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/run_lifting_suite.py
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

function(add_evm_solidity_patterns_suite test_name manifest_relpath)
	add_test(
		NAME ${test_name}
		COMMAND
			${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/run_evm_solidity_patterns_suite.py
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

function(add_evm_solidity_rewrite_suite test_name manifest_relpath)
	add_test(
		NAME ${test_name}
		COMMAND
			${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/run_evm_solidity_rewrite_suite.py
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

add_lifting_suite(
	notdec.lifting.wasm
	lifting/wasm/manifest.json
)

add_evm_solidity_patterns_suite(
	notdec.evm.solidity_patterns
	evm/solidity-patterns/manifest.json
)

add_evm_solidity_rewrite_suite(
	notdec.evm.solidity_rewrite
	evm/solidity-rewrite/manifest.json
)

add_type_recovery_suite(
	notdec.type_recovery.llvm_ir.tr_level_2
	type-recovery/llvm-ir/manifest.json
)

add_type_recovery_suite(
	notdec.type_recovery.sysy.tr_level_2
	type-recovery/sysy/manifest.json
)

add_type_recovery_suite(
	notdec.type_recovery.realworld.tr_level_2
	type-recovery/realworld/manifest.json
)
