# Request build options from user
option(TEST_RENDERER "Build renderer tests?" OFF)
option(BUILD_AEGISUB_CSRI "CRSI interface converts ASS data input to SSB." OFF)

# Reinterpret options for configuration
if(BUILD_AEGISUB_CSRI)
	set(USE_AEGISUB_INTERFACE 1)
else()
	set(USE_AEGISUB_INTERFACE 0)
endif()

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Plan dynamic library compiling
file(GLOB RENDERER_SOURCES *.cpp *.hpp *.h *.rc)
add_library(ssbrenderer SHARED ${RENDERER_SOURCES} $<TARGET_OBJECTS:ssbplugins>)

# Add library include directories
target_include_directories(ssbrenderer PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Add library links
target_link_libraries(ssbrenderer ssbparser)

# Setup library installation
install(TARGETS ssbrenderer
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
message(STATUS "Binaries installation path: ${CMAKE_INSTALL_PREFIX}/bin")
message(STATUS "Library installation path: ${CMAKE_INSTALL_PREFIX}/lib")
install(FILES public.h DESTINATION include RENAME ssbrenderer.h)
message(STATUS "Headers installation path: ${CMAKE_INSTALL_PREFIX}/include")
if(NOT MSVC)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/ssbrenderer.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/ssbrenderer.pc
		@ONLY
	)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ssbrenderer.pc DESTINATION share/pkgconfig)
	message(STATUS "Package condig installation path: ${CMAKE_INSTALL_PREFIX}/share/pkgconfig")
endif()

# Add functionality tests
if(${TEST_RENDERER})
endif()