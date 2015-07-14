# Request build options from user
option(TEST_GRAPHICS "Build graphics tests?" OFF)

# Plan static library compiling
file(GLOB GRAPHICS_SOURCES *.cpp *.hpp)
add_library(ssbgraphics STATIC ${GRAPHICS_SOURCES})

# Add functionality tests
if(${TEST_GRAPHICS})
	# Create transformation test
	add_executable(ssbgraphics_transform tests/transform.cpp)
	target_link_libraries(ssbgraphics_transform ssbgraphics)
	add_test(ssbgraphics_transform_test ssbgraphics_transform)
endif()