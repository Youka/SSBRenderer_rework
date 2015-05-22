# Compile static library
set(PLUGINS_SOURCES aegisub.cpp vapoursynth.cpp)
if(WIN32)
	set(PLUGINS_SOURCES ${PLUGINS_SOURCES} avisynth.cpp virtualdub.cpp)
endif()
if(MSVC)
	set(PLUGINS_SOURCES ${PLUGINS_SOURCES} dshow.cpp)
endif()
add_library(ssbplugins STATIC ${PLUGINS_SOURCES})

# Add functionality test
if(${BUILD_TESTS})
endif()