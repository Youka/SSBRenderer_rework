# Compile static library
file(GLOB PLUGINS_SOURCES *.cpp)
add_library(ssbplugins STATIC ${PLUGINS_SOURCES})

# Add functionality test
if(${BUILD_TESTS})
endif()