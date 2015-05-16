# Compile static library
add_library(ssbparser STATIC SSBParser.cpp)

# Add functionality test
if(${BUILD_TESTS})
	# Create minimal test
	add_executable(ssbparser_minimal tests/minimal.cpp)
	target_link_libraries(ssbparser_minimal ssbparser)
	add_test(ssbparser_minimal_test ssbparser_minimal)
endif()