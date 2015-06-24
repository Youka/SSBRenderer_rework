# Request build options from user
option(TEST_PLUGINS "Build plugins tests?" OFF)

# Request dependencies locations
if(MSVC)
	set(DEPEND_BASECLASSES_INCLUDE "" CACHE STRING "Header files directory of DirectShow BaseClasses")
	set(DEPEND_BASECLASSES_LIB "strmbase" CACHE STRING "Static library of DirectShow BaseClasses")
endif()

# Plan static library compiling
set(PLUGINS_SOURCES csri.cpp vapoursynth.cpp)
if(WIN32)
	set(PLUGINS_SOURCES ${PLUGINS_SOURCES} avisynth.cpp virtualdub.cpp)
endif()
if(MSVC)
	set(PLUGINS_SOURCES ${PLUGINS_SOURCES} dshow.cpp)
endif()
add_library(ssbplugins STATIC ${PLUGINS_SOURCES})
if(MSVC)
	target_include_directories(ssbplugins PUBLIC ${LIBRARY_BASECLASSES_INCLUDE})
	target_link_libraries(ssbplugins ${LIBRARY_BASECLASSES_LIB})
endif()

# Add functionality tests
if(${TEST_PLUGINS})
	# Create color inversion plugin
	add_library(ssbplugins_invert SHARED tests/invert.cpp)
	target_link_libraries(ssbplugins_invert ssbplugins)
	# Create CSRI test
	add_executable(ssbplugins_csri_host tests/csri_host.cpp)
	add_test(ssbplugins_csri_test ssbplugins_csri_host)
endif()