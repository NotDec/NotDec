
include(ExternalProject)
ExternalProject_Add(
    rapidjson
    PREFIX "vendor/rapidjson"
    GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
    GIT_TAG dfbe1db9da455552f7a9ad5d2aea17dd9d832ac1
    TIMEOUT 10
    CMAKE_ARGS
        -DRAPIDJSON_BUILD_TESTS=OFF
        -DRAPIDJSON_BUILD_DOC=OFF
        -DRAPIDJSON_BUILD_EXAMPLES=OFF
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
	SOURCE_DIR ${CMAKE_BINARY_DIR}/rapidjson
	BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/rapidjson
)

# Prepare RapidJSON (RapidJSON is a header-only library)
ExternalProject_Get_Property(rapidjson source_dir)
set(RAPIDJSON_INCLUDE_DIR ${source_dir}/include)

add_library(llvmir2hll STATIC
	analysis/alias_analysis/alias_analyses/basic_alias_analysis.cpp
	analysis/alias_analysis/alias_analyses/simple_alias_analysis.cpp
	analysis/alias_analysis/alias_analysis.cpp
	analysis/break_in_if_analysis.cpp
	analysis/def_use_analysis.cpp
	analysis/expr_types_analysis.cpp
	analysis/goto_target_analysis.cpp
	analysis/indirect_func_ref_analysis.cpp
	analysis/no_init_var_def_analysis.cpp
	analysis/null_pointer_analysis.cpp
	analysis/special_fp_analysis.cpp
	analysis/use_def_analysis.cpp
	analysis/used_types_visitor.cpp
	analysis/used_vars_visitor.cpp
	analysis/value_analysis.cpp
	analysis/var_uses_visitor.cpp
	analysis/written_into_globals_visitor.cpp
	config/config.cpp
	config/configs/json_config.cpp
	evaluator/arithm_expr_evaluator.cpp
	evaluator/arithm_expr_evaluators/c_arithm_expr_evaluator.cpp
	evaluator/arithm_expr_evaluators/strict_arithm_expr_evaluator.cpp
	graphs/cfg/cfg.cpp
	graphs/cfg/cfg_builder.cpp
	graphs/cfg/cfg_builders/non_recursive_cfg_builder.cpp
	graphs/cfg/cfg_builders/recursive_cfg_builder.cpp
	graphs/cfg/cfg_traversal.cpp
	graphs/cfg/cfg_traversals/lhs_rhs_uses_cfg_traversal.cpp
	graphs/cfg/cfg_traversals/modified_before_read_cfg_traversal.cpp
	graphs/cfg/cfg_traversals/no_var_def_cfg_traversal.cpp
	graphs/cfg/cfg_traversals/nodes_of_var_use_cfg_traversal.cpp
	graphs/cfg/cfg_traversals/optim_func_info_cfg_traversal.cpp
	graphs/cfg/cfg_traversals/var_def_cfg_traversal.cpp
	graphs/cfg/cfg_traversals/var_use_cfg_traversal.cpp
	graphs/cfg/cfg_writer.cpp
	graphs/cfg/cfg_writers/graphviz_cfg_writer.cpp
	graphs/cg/cg.cpp
	graphs/cg/cg_builder.cpp
	graphs/cg/cg_writer.cpp
	graphs/cg/cg_writers/graphviz_cg_writer.cpp
	hll/bir_writer.cpp
	hll/bracket_manager.cpp
	hll/bracket_managers/c_bracket_manager.cpp
	hll/bracket_managers/no_bracket_manager.cpp
	hll/compound_op_manager.cpp
	hll/compound_op_managers/c_compound_op_manager.cpp
	hll/compound_op_managers/no_compound_op_manager.cpp
	hll/hll_writer.cpp
	hll/hll_writers/c_hll_writer.cpp
	hll/output_manager.cpp
	hll/output_managers/json_manager.cpp
	hll/output_managers/plain_manager.cpp
	ir/add_op_expr.cpp
	ir/address_op_expr.cpp
	ir/and_op_expr.cpp
	ir/array_index_op_expr.cpp
	ir/array_type.cpp
	ir/assign_op_expr.cpp
	ir/assign_stmt.cpp
	ir/binary_op_expr.cpp
	ir/bit_and_op_expr.cpp
	ir/bit_cast_expr.cpp
	ir/bit_or_op_expr.cpp
	ir/bit_shl_op_expr.cpp
	ir/bit_shr_op_expr.cpp
	ir/bit_xor_op_expr.cpp
	ir/break_stmt.cpp
	ir/call_expr.cpp
	ir/call_stmt.cpp
	ir/cast_expr.cpp
	ir/comma_op_expr.cpp
	ir/const_array.cpp
	ir/const_bool.cpp
	ir/const_float.cpp
	ir/const_int.cpp
	ir/const_null_pointer.cpp
	ir/const_string.cpp
	ir/const_struct.cpp
	ir/const_symbol.cpp
	ir/continue_stmt.cpp
	ir/deref_op_expr.cpp
	ir/div_op_expr.cpp
	ir/empty_stmt.cpp
	ir/eq_op_expr.cpp
	ir/expression.cpp
	ir/ext_cast_expr.cpp
	ir/float_type.cpp
	ir/for_loop_stmt.cpp
	ir/fp_to_int_cast_expr.cpp
	ir/function.cpp
	ir/function_builder.cpp
	ir/function_type.cpp
	ir/global_var_def.cpp
	ir/goto_stmt.cpp
	ir/gt_eq_op_expr.cpp
	ir/gt_op_expr.cpp
	ir/if_stmt.cpp
	ir/int_to_fp_cast_expr.cpp
	ir/int_to_ptr_cast_expr.cpp
	ir/int_type.cpp
	ir/lt_eq_op_expr.cpp
	ir/lt_op_expr.cpp
	ir/mod_op_expr.cpp
	ir/module.cpp
	ir/mul_op_expr.cpp
	ir/neg_op_expr.cpp
	ir/neq_op_expr.cpp
	ir/not_op_expr.cpp
	ir/or_op_expr.cpp
	ir/pointer_type.cpp
	ir/ptr_to_int_cast_expr.cpp
	ir/return_stmt.cpp
	ir/statement.cpp
	ir/string_type.cpp
	ir/struct_index_op_expr.cpp
	ir/struct_type.cpp
	ir/sub_op_expr.cpp
	ir/switch_stmt.cpp
	ir/ternary_op_expr.cpp
	ir/trunc_cast_expr.cpp
	ir/ufor_loop_stmt.cpp
	ir/unary_op_expr.cpp
	ir/unknown_type.cpp
	ir/unreachable_stmt.cpp
	ir/value.cpp
	ir/var_def_stmt.cpp
	ir/variable.cpp
	ir/void_type.cpp
	ir/while_loop_stmt.cpp
	llvm/llvm_debug_info_obtainer.cpp
	llvm/llvm_intrinsic_converter.cpp
	llvm/llvm_support.cpp
	llvm/llvmir2bir_converter.cpp
	llvm/llvmir2bir_converter/basic_block_converter.cpp
	llvm/llvmir2bir_converter/cfg_node.cpp
	llvm/llvmir2bir_converter/labels_handler.cpp
	llvm/llvmir2bir_converter/llvm_constant_converter.cpp
	llvm/llvmir2bir_converter/llvm_fcmp_converter.cpp
	llvm/llvmir2bir_converter/llvm_instruction_converter.cpp
	llvm/llvmir2bir_converter/llvm_type_converter.cpp
	llvm/llvmir2bir_converter/llvm_value_converter.cpp
	llvm/llvmir2bir_converter/structure_converter.cpp
	llvm/llvmir2bir_converter/variables_manager.cpp
	llvm/string_conversions.cpp
	obtainer/call_info_obtainer.cpp
	obtainer/call_info_obtainers/optim_call_info_obtainer.cpp
	obtainer/call_info_obtainers/pessim_call_info_obtainer.cpp
	obtainer/calls_in_module_obtainer.cpp
	obtainer/calls_obtainer.cpp
	optimizer/func_optimizer.cpp
	optimizer/optimizer.cpp
	optimizer/optimizer_manager.cpp
	optimizer/optimizers/bit_op_to_log_op_optimizer.cpp
	optimizer/optimizers/bit_shift_optimizer.cpp
	optimizer/optimizers/break_continue_return_optimizer.cpp
	optimizer/optimizers/c_array_arg_optimizer.cpp
	optimizer/optimizers/c_cast_optimizer.cpp
	optimizer/optimizers/copy_propagation_optimizer.cpp
	optimizer/optimizers/dead_code_optimizer.cpp
	optimizer/optimizers/dead_local_assign_optimizer.cpp
	optimizer/optimizers/deref_address_optimizer.cpp
	optimizer/optimizers/deref_to_array_index_optimizer.cpp
	optimizer/optimizers/empty_array_to_string_optimizer.cpp
	optimizer/optimizers/empty_stmt_optimizer.cpp
	optimizer/optimizers/goto_stmt_optimizer.cpp
	optimizer/optimizers/if_before_loop_optimizer.cpp
	optimizer/optimizers/if_structure_optimizer.cpp
	optimizer/optimizers/if_to_switch_optimizer.cpp
	optimizer/optimizers/llvm_intrinsics_optimizer.cpp
	optimizer/optimizers/loop_last_continue_optimizer.cpp
	optimizer/optimizers/no_init_var_def_optimizer.cpp
	optimizer/optimizers/pre_while_true_loop_conv_optimizer.cpp
	optimizer/optimizers/remove_all_casts_optimizer.cpp
	optimizer/optimizers/remove_useless_casts_optimizer.cpp
	optimizer/optimizers/self_assign_optimizer.cpp
	optimizer/optimizers/simple_copy_propagation_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/bool_comparison_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/change_order_of_operands_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/const_operator_const_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/equal_operands_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/negation_operator_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/negative_operand_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/one_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/ternary_operator_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/three_operands_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr/zero_sub_optimizer.cpp
	optimizer/optimizers/simplify_arithm_expr_optimizer.cpp
	optimizer/optimizers/unused_global_var_optimizer.cpp
	optimizer/optimizers/var_def_for_loop_optimizer.cpp
	optimizer/optimizers/var_def_stmt_optimizer.cpp
	optimizer/optimizers/void_return_optimizer.cpp
	optimizer/optimizers/while_true_to_for_loop_optimizer.cpp
	optimizer/optimizers/while_true_to_ufor_loop_optimizer.cpp
	optimizer/optimizers/while_true_to_while_cond_optimizer.cpp
	pattern/pattern_finder.cpp
	pattern/pattern_finder_runner.cpp
	pattern/pattern_finder_runners/cli_pattern_finder_runner.cpp
	pattern/pattern_finder_runners/no_action_pattern_finder_runner.cpp
	pattern/pattern_finders/api_call/api_call_info.cpp
	pattern/pattern_finders/api_call/api_call_info_seq.cpp
	pattern/pattern_finders/api_call/api_call_info_seq_parser.cpp
	pattern/pattern_finders/api_call/api_call_seq_data.cpp
	pattern/pattern_finders/api_call/api_call_seq_finder.cpp
	pattern/pattern_finders/api_call/api_call_seq_finders/basic_block_api_call_seq_finder.cpp
	pattern/pattern_finders/api_call_pattern_finder.cpp
	pattern/pattern_finders/api_call_seq_pattern_finder.cpp
	pattern/patterns/stmts_pattern.cpp
	semantics/semantics/compound_semantics.cpp
	semantics/semantics/compound_semantics_builder.cpp
	semantics/semantics/default_semantics.cpp
	semantics/semantics/gcc_general_semantics.cpp
	semantics/semantics/gcc_general_semantics/get_c_header_file_for_func.cpp
	semantics/semantics/gcc_general_semantics/get_name_of_param.cpp
	semantics/semantics/gcc_general_semantics/get_name_of_var_storing_result.cpp
	semantics/semantics/gcc_general_semantics/get_symbolic_names_for_param.cpp
	semantics/semantics/impl_support/get_c_header_file_for_func.cpp
	semantics/semantics/impl_support/get_name_of_param.cpp
	semantics/semantics/impl_support/get_name_of_var_storing_result.cpp
	semantics/semantics/impl_support/get_symbolic_names_for_param.cpp
	semantics/semantics/libc_semantics.cpp
	semantics/semantics/libc_semantics/func_never_returns.cpp
	semantics/semantics/libc_semantics/get_c_header_file_for_func.cpp
	semantics/semantics/libc_semantics/get_name_of_param.cpp
	semantics/semantics/libc_semantics/get_name_of_var_storing_result.cpp
	semantics/semantics/libc_semantics/get_symbolic_names_for_param.cpp
	semantics/semantics/win_api_semantics.cpp
	semantics/semantics/win_api_semantics/func_never_returns.cpp
	semantics/semantics/win_api_semantics/get_c_header_file_for_func.cpp
	semantics/semantics/win_api_semantics/get_name_of_param.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/a.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/b.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/c1.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/c2.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/d.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/e.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/f.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/g1.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/g2.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/h.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/i.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/j.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/k.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/l.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/m.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/n.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/o.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/p.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/q.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/r.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/s.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/t.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/u.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/v.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/w.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/x.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/y.cpp
	semantics/semantics/win_api_semantics/get_name_of_param/z.cpp
	semantics/semantics/win_api_semantics/get_name_of_var_storing_result.cpp
	semantics/semantics/win_api_semantics/get_symbolic_names_for_param.cpp
	support/const_symbol_converter.cpp
	support/expr_types_fixer.cpp
	support/expression_negater.cpp
	support/global_vars_sorter.cpp
	support/headers_for_declared_funcs.cpp
	support/library_funcs_remover.cpp
	support/statements_counter.cpp
	support/struct_types_sorter.cpp
	support/types.cpp
	support/unreachable_code_in_cfg_remover.cpp
	support/valid_state.cpp
	support/value_text_repr_visitor.cpp
	support/variable_replacer.cpp
	support/visitors/ordered_all_visitor.cpp
	utils/graphviz.cpp
	utils/ir.cpp
	utils/loop_optimizer.cpp
	utils/string.cpp
	validator/validator.cpp
	validator/validators/break_outside_loop_validator.cpp
	validator/validators/no_global_var_def_validator.cpp
	validator/validators/return_validator.cpp
	var_name_gen/var_name_gen.cpp
	var_name_gen/var_name_gens/fruit_var_name_gen.cpp
	var_name_gen/var_name_gens/num_var_name_gen.cpp
	var_name_gen/var_name_gens/word_var_name_gen.cpp
	var_renamer/var_renamer.cpp
	var_renamer/var_renamers/address_var_renamer.cpp
	var_renamer/var_renamers/hungarian_var_renamer.cpp
	var_renamer/var_renamers/readable_var_renamer.cpp
	var_renamer/var_renamers/simple_var_renamer.cpp
	var_renamer/var_renamers/unified_var_renamer.cpp
	common/calling_convention.cpp
	common/address.cpp
	common/class.cpp
	common/tool_info.cpp
	common/file_format.cpp
	common/object.cpp
	common/architecture.cpp
	common/file_type.cpp
	common/basic_block.cpp
	common/storage.cpp
	common/type.cpp
	common/function.cpp
	common/language.cpp
	common/vtable.cpp
	common/pattern.cpp
	retdec-config/parameters.cpp
	retdec-config/config.cpp
	serdes/calling_convention.cpp
	serdes/address.cpp
	serdes/class.cpp
	serdes/tool_info.cpp
	serdes/file_format.cpp
	serdes/object.cpp
	serdes/architecture.cpp
	serdes/file_type.cpp
	serdes/basic_block.cpp
	serdes/storage.cpp
	serdes/type.cpp
	serdes/function.cpp
	serdes/language.cpp
	serdes/vtable.cpp
	serdes/pattern.cpp
	serdes/std.cpp
	retdec-utils/binary_path.cpp
	retdec-utils/memory.cpp
	retdec-utils/crc32.cpp
	retdec-utils/ord_lookup.cpp
	retdec-utils/time.cpp
	retdec-utils/math.cpp
	retdec-utils/conversion.cpp
	retdec-utils/alignment.cpp
	retdec-utils/file_io.cpp
	retdec-utils/byte_value_storage.cpp
	retdec-utils/system.cpp
	retdec-utils/dynamic_buffer.cpp
	retdec-utils/io/logger.cpp
	retdec-utils/io/log.cpp
	retdec-utils/version.cpp
	retdec-utils/string.cpp
	llvmir2hll.cpp
	config.cpp
)
add_library(retdec::llvmir2hll ALIAS llvmir2hll)

target_compile_features(llvmir2hll PUBLIC cxx_std_17)

target_include_directories(llvmir2hll
	PUBLIC
	${RAPIDJSON_INCLUDE_DIR}
)
add_dependencies(llvmir2hll rapidjson)

# target_link_libraries(llvmir2hll
# 	PUBLIC
# 		retdec::config
# 		retdec::utils
# 		retdec::deps::rapidjson
# 		retdec::deps::llvm
# )

# We need to compile source files with /bigobj to prevent the following
# kind of an error:
#
#   fatal error C1128: number of sections exceeded object file format
#                      limit: compile with /bigobj
if(MSVC)
	target_compile_options(llvmir2hll PUBLIC "/bigobj")
endif()

target_compile_options(llvmir2hll PRIVATE 
	"-frtti"
	"-DRAPIDJSON_HAS_CXX11_RVALUE_REFS=1"
	"-DRAPIDJSON_HAS_STDSTRING=1")

set_target_properties(llvmir2hll
	PROPERTIES
		OUTPUT_NAME "retdec-llvmir2hll"
)

# # Install includes.
# install(
# 	DIRECTORY ${RETDEC_INCLUDE_DIR}/retdec/llvmir2hll
# 	DESTINATION ${RETDEC_INSTALL_INCLUDE_DIR}/retdec
# )

# # Install libs.
# install(TARGETS llvmir2hll
# 	EXPORT llvmir2hll-targets
# 	ARCHIVE DESTINATION ${RETDEC_INSTALL_LIB_DIR}
# 	LIBRARY DESTINATION ${RETDEC_INSTALL_LIB_DIR}
# )
