enable_testing()

add_executable(
	GraphTest
	GraphTest.cpp
)
target_link_libraries(
	GraphTest
	GTest::gtest_main
	notdec
)

include(GoogleTest)
gtest_discover_tests(GraphTest)
