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

# Setup library installation
install(TARGETS ssbrenderer
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(FILES public.h DESTINATION include RENAME ssb.h)
if(NOT MSVC)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/ssbrenderer.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/ssbrenderer.pc
		@ONLY
	)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ssbrenderer.pc DESTINATION share/pkgconfig)
endif()

# Add functionality tests
if(${TEST_RENDERER})
endif()