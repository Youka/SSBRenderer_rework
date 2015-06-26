# Request build options from user
option(TEST_GRAPHICS "Build graphics tests?" OFF)

# Plan static library compiling
file(GLOB GRAPHICS_SOURCES *.cpp)
add_library(ssbgraphics STATIC ${GRAPHICS_SOURCES})

# Add functionality tests
if(${TEST_GRAPHICS})
endif()