# Request build options from user
set(BUILD_FONT_PRECISION 64 CACHE STRING "Internal font up- & downscale for better calculation results.")
set(DEPEND_LIBDIVIDE "" CACHE PATH "Libdivide directory (containing header file).")
option(TEST_GRAPHICS "Build graphics tests?" OFF)

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Plan static library compiling
set(GRAPHICS_SOURCES blend.cpp blur.cpp flip.cpp matrix.cpp simd.h threads.hpp gutils.hpp)
if(WIN32)
	set(GRAPHICS_SOURCES ${GRAPHICS_SOURCES} text_win.cpp)
else()
	set(GRAPHICS_SOURCES ${GRAPHICS_SOURCES} text_unix.cpp)
endif()
add_library(ssbgraphics STATIC ${GRAPHICS_SOURCES})

# Add library include directories
target_include_directories(ssbgraphics PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${DEPEND_LIBDIVIDE})

# Add library links
if(WIN32)
	target_link_libraries(ssbgraphics usp10)
else()
	target_link_libraries(ssbgraphics cairo pango-1.0 pangocairo-1.0)
endif()

# Add functionality tests
if(${TEST_GRAPHICS})
	# Create transformation test
	add_executable(ssbgraphics_transform tests/transform.cpp)
	target_link_libraries(ssbgraphics_transform ssbgraphics)
	add_test(ssbgraphics_transform_test ssbgraphics_transform)
	# Create flip & blend test
	add_executable(ssbgraphics_flipblend tests/flipblend.cpp tests/tga.hpp tests/test_images.h)
	target_link_libraries(ssbgraphics_flipblend ssbgraphics)
	add_test(ssbgraphics_flipblend_test ssbgraphics_flipblend)
	# Create blur test
	add_executable(ssbgraphics_blur tests/blurred.cpp tests/tga.hpp tests/test_images.h)
	target_link_libraries(ssbgraphics_blur ssbgraphics)
	add_test(ssbgraphics_blur_test ssbgraphics_blur 2.5 8.2)
	# Create font test
	add_executable(ssbgraphics_text tests/text.cpp)
	target_link_libraries(ssbgraphics_text ssbgraphics)
	add_test(ssbgraphics_text_test ssbgraphics_text)
endif()