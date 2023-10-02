# ======== custom target to generate souffle c++ source ==================
# Used in outer project
add_custom_target(souffle_cpp DEPENDS 
                "rules/pointer_main.cpp")

add_custom_command(OUTPUT "rules/pointer_main.cpp"
    COMMAND souffle -g pointer_main.cpp pointer_main.dl
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/rules
    VERBATIM
)

# ======== custom target to generate fact-names.def and fact-names.h ==================
add_custom_target(fact_names DEPENDS "${CMAKE_CURRENT_LIST_DIR}/fact-names.def" "${CMAKE_CURRENT_LIST_DIR}/fact-names.h")

add_custom_command(OUTPUT "${CMAKE_CURRENT_LIST_DIR}/fact-names.def" "${CMAKE_CURRENT_LIST_DIR}/fact-names.h"
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/gen_header.py
    VERBATIM
)

# ======== notdec-datalog library ==================
add_library(notdec-datalog STATIC
	# source files that are common to all frontend
    fact-generator.cpp
)

target_link_libraries(notdec-datalog
    PUBLIC
    LLVMCore LLVMIRReader LLVMSupport LLVMPasses
)

add_dependencies(notdec-datalog fact_names)

# use souffle 64bit domain
target_compile_definitions(notdec-datalog PUBLIC RAM_DOMAIN_SIZE=64)

# main
add_executable(notdec-factgen
	fact-generator-main.cpp
)

target_link_libraries(notdec-factgen
    PUBLIC
    notdec-datalog
)
