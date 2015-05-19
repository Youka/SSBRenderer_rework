# Request build options from user
set(BUILD_MAX_INLINE_STYLES 64 CACHE STRING "Maximal number of insertions of a style into a line" FORCE)

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Compile static library
file(GLOB PARSER_SOURCES *.cpp)
add_library(ssbparser STATIC ${PARSER_SOURCES})

# Add library include directories
target_include_directories(ssbparser PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Add functionality test
if(${BUILD_TESTS})
	# Create minimal test
	add_executable(ssbparser_minimal tests/minimal.cpp)
	target_link_libraries(ssbparser_minimal ssbparser)
	add_test(ssbparser_minimal_test ssbparser_minimal)
	# Create simple test
	add_executable(ssbparser_simple tests/simple.cpp)
	target_link_libraries(ssbparser_simple ssbparser)
	add_test(ssbparser_simple_test ssbparser_simple "#META" "Width: 100")
endif()