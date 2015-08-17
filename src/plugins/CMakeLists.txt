# Request build options from user
option(TEST_PLUGINS "Build plugins tests?" OFF)

# Plan static library compiling
set(PLUGINS_SOURCES FilterBase.hpp csri.cpp vapoursynth.cpp)
if(WIN32)
	set(PLUGINS_SOURCES ${PLUGINS_SOURCES} avisynth.cpp virtualdub.cpp mediaf.cpp)
endif()
add_library(ssbplugins OBJECT ${PLUGINS_SOURCES})

# Add functionality tests
if(${TEST_PLUGINS})
	# Create color inversion plugin
	add_library(ssbplugins_invert SHARED tests/invert.cpp $<TARGET_OBJECTS:ssbplugins>)
	if(WIN32)
		target_link_libraries(ssbplugins_invert mfuuid strmiids mfplat mf evr)
	endif()
	# Create CSRI test
	add_executable(ssbplugins_csri_host tests/csri_host.cpp)
	add_test(ssbplugins_csri_test ssbplugins_csri_host)
endif()