# Check for available compilers (has to happen after project definition for compiler flags)
if(NOT(CMAKE_COMPILER_IS_GNUCC OR MSVC_VERSION GREATER 1600))
	message(FATAL_ERROR "Just GCC & MSVC11+ compilers supported!")
endif()
message(STATUS "Generator: ${CMAKE_GENERATOR}")

# Request build options from user
if(MINGW)
	set(BUILD_STD_STATIC_VALUE ON)
else()
	set(BUILD_STD_STATIC_VALUE OFF)
endif()
option(BUILD_STD_STATIC "Link standard libraries statically?" ${BUILD_STD_STATIC_VALUE})
if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Include script helpers
include(${PROJECT_SOURCE_DIR}/cmake/linker_flags.cmake)

# Set compile warnings
if(MSVC)
	set(COMPILE_WARNINGS /W2)
else()
	set(COMPILE_WARNINGS -Wall -Wextra -pedantic -Wredundant-decls -Wunreachable-code -Wmissing-include-dirs -Wswitch-enum)
endif()
add_compile_options(${COMPILE_WARNINGS})
message(STATUS "Compile warnings: ${COMPILE_WARNINGS}")

# Add C++11 & vectorization support
if(CMAKE_COMPILER_IS_GNUCC)
	set(COMPILE_FEATURES -std=c++11 -mavx)
else()
	set(COMPILE_FEATURES /arch:AVX)
endif()
add_compile_options(${COMPILE_FEATURES})
message(STATUS "Additional compiler features: ${COMPILE_FEATURES}")

# Set link time optimizations
if(MSVC)
	set(LINK_TIME_OPTIMIZATIONS /LTCG)
elseif(NOT(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
	set(LINK_TIME_OPTIMIZATIONS -s)
endif()
add_linker_flags(${LINK_TIME_OPTIMIZATIONS})
message(STATUS "Link time optimizations: ${LINK_TIME_OPTIMIZATIONS}")

# Set standard libraries link type (static vs dynamic)
if(${BUILD_STD_STATIC})
	if(CMAKE_COMPILER_IS_GNUCC)
		add_linker_flags("-static")
	else()	# MSVC
		foreach(build_type
			CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
			CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
			if(${build_type} MATCHES /MD)
				string(REPLACE /MD /MT ${build_type} "${${build_type}}")
			endif()
		endforeach()
	endif()
	message(STATUS "Standard libraries link type: static")
else()
	message(STATUS "Standard libraries link type: dynamic")
endif()

# Process parser
add_subdirectory(parser)
message(STATUS "Processed parser")

# Process plugin interfaces
add_subdirectory(plugins)
message(STATUS "Processed plugin interfaces")

#Process graphics utilities
add_subdirectory(graphics)
message(STATUS "Processed graphics utilities")

# Process renderer
add_subdirectory(renderer)
message(STATUS "Processed renderer")