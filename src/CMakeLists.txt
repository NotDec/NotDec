
add_executable(notdec 
	notdec.cpp
)

# target_compile_features(bin2pat PUBLIC cxx_std_17)

target_link_libraries(notdec 
	LLVMCore # LLVMPasses LLVMIRReader LLVMSupport
)

set_target_properties(notdec
	PROPERTIES
		OUTPUT_NAME "notdec"
)

install(TARGETS notdec DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
