# Request dependencies locations
if(MSVC)
	set(DEPEND_BASECLASSES_INCLUDE "" CACHE STRING "Header files directory of DirectShow BaseClasses")
	set(DEPEND_BASECLASSES_LIB "strmbase" CACHE STRING "Static library of DirectShow BaseClasses")
endif()

# Compile static library
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

# Add functionality test
if(${BUILD_TESTS})
endif()