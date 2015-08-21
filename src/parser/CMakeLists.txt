# Request build options from user
set(BUILD_MAX_INLINE_STYLES 64 CACHE STRING "Maximal number of insertions of a style into a SSB line.")
option(TEST_PARSER "Build parser tests?" OFF)

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Plan static library compiling
file(GLOB PARSER_SOURCES *.cpp *.hpp)
add_library(ssbparser STATIC ${PARSER_SOURCES})

# Add library include directories
target_include_directories(ssbparser PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Add functionality tests
if(${TEST_PARSER})
	# Create minimal test
	add_executable(ssbparser_minimal tests/minimal.cpp)
	target_link_libraries(ssbparser_minimal ssbparser)
	add_test(ssbparser_minimal_test ssbparser_minimal)
	# Create simple test
	add_executable(ssbparser_simple tests/simple.cpp)
	target_link_libraries(ssbparser_simple ssbparser)
	add_test(ssbparser_simple_test ssbparser_simple "#FRAME" "Width: 100" "#EVENTS" "0-1:0.0|||{an=2;pos=200.5,100}Test")
endif()