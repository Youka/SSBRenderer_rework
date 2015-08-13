# Request build options from user
option(TEST_RENDERER "Build renderer tests?" OFF)

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Plan dynamic library compiling
file(GLOB RENDERER_SOURCES *.cpp *.hpp *.h *.rc)
add_library(ssbrenderer SHARED ${RENDERER_SOURCES})

# Add library include directories
target_include_directories(ssbrenderer PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Add functionality tests
if(${TEST_RENDERER})
endif()