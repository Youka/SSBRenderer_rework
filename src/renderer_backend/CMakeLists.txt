# Request build options from user
option(BUILD_HW_ACCEL "Use GPU for graphics rendering?" ON)
set(DEPEND_GLFW_INC "" CACHE PATH "GLFW include directory (just needed when BUILD_HW_ACCEL is ON).")
set(DEPEND_GLFW_LIB "" CACHE FILEPATH "GLFW library filepath (just needed when BUILD_HW_ACCEL is ON).")
set(DEPEND_Z_LIB "" CACHE FILEPATH "Zlib library filepath (just needed when BUILD_HW_ACCEL is OFF).")
set(DEPEND_PNG_INC "" CACHE PATH "Libpng include directory (just needed when BUILD_HW_ACCEL is ON).")
set(DEPEND_PNG_LIB "" CACHE FILEPATH "Libpng library filepath (just needed when BUILD_HW_ACCEL is OFF).")
set(DEPEND_PIXMAN_LIB "" CACHE FILEPATH "Pixman library filepath (just needed when BUILD_HW_ACCEL is OFF).")
set(DEPEND_CAIRO_INC "" CACHE PATH "Cairo include directory (just needed when BUILD_HW_ACCEL is OFF).")
set(DEPEND_CAIRO_LIB "" CACHE FILEPATH "Cairo library filepath (just needed when BUILD_HW_ACCEL is OFF).")
option(TEST_RENDERER_BACKEND "Build renderer backend tests?" OFF)

# Plan static library compiling
set(RENDERER_BACKEND_SOURCES Renderer.hpp)
if(BUILD_HW_ACCEL)
	set(RENDERER_BACKEND_SOURCES ${RENDERER_BACKEND_SOURCES} Renderer_ogl.cpp)
else()
	set(RENDERER_BACKEND_SOURCES ${RENDERER_BACKEND_SOURCES} Renderer_cairo.cpp)
endif()
add_library(ssbrenderer_backend STATIC ${RENDERER_BACKEND_SOURCES})

# Add library include directories
if(BUILD_HW_ACCEL)
	target_include_directories(ssbrenderer_backend PUBLIC ${DEPEND_GLFW_INC} ${DEPEND_PNG_INC})
else()
	target_include_directories(ssbrenderer_backend PUBLIC ${DEPEND_CAIRO_INC})
endif()

# Add library links
set(RENDERER_BACKEND_LINKS ${DEPEND_PNG_LIB} ${DEPEND_Z_LIB})
if(BUILD_HW_ACCEL)
	include(FindOpenGL)
	set(RENDERER_BACKEND_LINKS ${DEPEND_GLFW_LIB} ${RENDERER_BACKEND_LINKS} ${OPENGL_LIBRARIES})
else()
	set(RENDERER_BACKEND_LINKS ${DEPEND_CAIRO_LIB} ${DEPEND_PIXMAN_LIB} ${RENDERER_BACKEND_LINKS})
endif()
target_link_libraries(ssbrenderer_backend ${RENDERER_BACKEND_LINKS})

# Add functionality tests
if(${TEST_RENDERER_BACKEND})
endif()