enable_testing()

add_executable(
	MLsubGeneratorTest
	MLsubGeneratorTest.cpp
)
target_link_libraries(
	MLsubGeneratorTest
	GTest::gtest_main
	notdec
)

add_executable(
	TypeBuilderTest
	TypeBuilderTest.cpp
)
target_link_libraries(
	TypeBuilderTest
	GTest::gtest_main
	notdec
)

add_executable(
	PAPathTest
	PAPathTest.cpp
)
target_link_libraries(
	PAPathTest
	GTest::gtest_main
	notdec
)

include(GoogleTest)
gtest_discover_tests(MLsubGeneratorTest)
gtest_discover_tests(TypeBuilderTest)
gtest_discover_tests(PAPathTest)
