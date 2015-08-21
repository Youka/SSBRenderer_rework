# Request build options from user
option(TEST_RENDERER_BACKEND "Build renderer backend tests?" OFF)
option(BUILD_HW_ACCEL "Use GPU for graphics rendering?" ON)

# Plan static library compiling
file(GLOB RENDERER_BACKEND_SOURCES *.cpp *.hpp)
add_library(ssbrenderer_backend STATIC ${RENDERER_BACKEND_SOURCES})

# Add library definitions
if(BUILD_HW_ACCEL)
	target_compile_definitions(ssbrenderer_backend PUBLIC USE_GPU)
endif()

# Add functionality tests
if(${TEST_RENDERER_BACKEND})
endif()