# Request build options from user
option(TEST_STRINGS "Build strings tests?" OFF)

# Just add sources to project files
file(GLOB STRINGS_SOURCES *.hpp)
add_custom_target(ssbstrings SOURCES ${STRINGS_SOURCES})

# Add functionality tests
if(${TEST_STRINGS})
endif()