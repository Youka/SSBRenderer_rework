# Request build options from user
option(TEST_STRINGS "Build strings tests?" OFF)

# Just add sources to project files
file(GLOB STRINGS_SOURCES *.hpp)
add_custom_target(ssbstrings SOURCES ${STRINGS_SOURCES})

# Add functionality tests
if(${TEST_STRINGS})
	# Create utf-8 test
	add_executable(ssbstrings_utf8 tests/utf8.cpp)
	add_test(ssbstrings_utf8_test ssbstrings_utf8)
	# Create basic strings test
	add_executable(ssbstrings_basic tests/basic.cpp)
	add_test(ssbstrings_basic_test ssbstrings_basic)
endif()