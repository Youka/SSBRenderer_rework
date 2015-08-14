# Request build options from user
option(TEST_UTILS "Build utils tests?" OFF)

# Just add sources to project files
file(GLOB UTILS_SOURCES *.hpp)
add_custom_target(ssbutils SOURCES ${UTILS_SOURCES})

# Add functionality tests
if(${TEST_UTILS})
	# Create utf-8 test
	add_executable(ssbutils_utf8 tests/utf8.cpp)
	add_test(ssbutils_utf8_test ssbutils_utf8)
	# Create string test
	add_executable(ssbutils_string tests/string.cpp)
	add_test(ssbutils_string_test ssbutils_string)
endif()