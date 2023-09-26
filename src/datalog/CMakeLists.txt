# ======== custom target to generate souffle c++ source ==================

add_custom_target(souffle_cpp DEPENDS 
                "rules/pointer_main.cpp")

add_custom_command(OUTPUT "rules/pointer_main.cpp"
    COMMAND souffle -g pointer_main.cpp pointer_main.dl
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/rules
)

# ======== custom target to generate fact-names.def and fact-names.h ==================
add_custom_target(fact_names DEPENDS "fact-names.def" "fact-names.h")

add_custom_command(OUTPUT "fact-names.def" "fact-names.h"
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/gen_header.py
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

# main
add_executable(notdec-factgen
	fact-generator-main.cpp
)

target_link_libraries(notdec-factgen
    PUBLIC
    notdec-datalog
)
