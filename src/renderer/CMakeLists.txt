# Request build options from user
option(BUILD_AEGISUB_CSRI "Should the CRSI interface convert ASS data input to SSB?" OFF)
set(BUILD_CACHE_SIZE 64 CACHE STRING "Maximal number of cached objects.")
set(DEPEND_MUPARSER_INC "" CACHE PATH "muParser include directory.")
set(DEPEND_MUPARSER_LIB "" CACHE FILEPATH "muParser library filepath.")
option(TEST_RENDERER "Build renderer tests?" OFF)

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Plan dynamic library compiling
file(GLOB RENDERER_SOURCES *.cpp *.hpp *.h *.rc)
add_library(ssbrenderer SHARED ${RENDERER_SOURCES} $<TARGET_OBJECTS:ssbplugins>)

# Add library include directories
target_include_directories(ssbrenderer PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${DEPEND_MUPARSER_INC})

# Add library definitions
if(BUILD_AEGISUB_CSRI)
	target_compile_definitions(ssbrenderer PUBLIC USE_AEGISUB_INTERFACE)
endif()

# Add library links
set(RENDERER_LINKS ssbparser ssbrenderer_backend ssbgraphics ${DEPEND_MUPARSER_LIB})
if(WIN32)
	set(RENDERER_LINKS ${RENDERER_LINKS} mfuuid mfplat)
endif()
target_link_libraries(ssbrenderer ${RENDERER_LINKS})

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
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/ssbrenderer.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/ssbrenderer.pc
	@ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ssbrenderer.pc DESTINATION share/pkgconfig)
message(STATUS "Package condig installation path: ${CMAKE_INSTALL_PREFIX}/share/pkgconfig")

# Add functionality tests
if(${TEST_RENDERER})
	# Create minimal test
	add_executable(ssbrenderer_minimal tests/minimal.c)
	target_link_libraries(ssbrenderer_minimal ssbrenderer)
	add_test(ssbrenderer_minimal_test ssbrenderer_minimal)
endif()