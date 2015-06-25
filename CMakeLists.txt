# Check for cmake minimum required version (used by development)
cmake_minimum_required(VERSION 3.0)
message(STATUS "CMake 3.0+: ${CMAKE_VERSION}")

# Set project header
project(SSBRenderer)
set(PROJECT_AUTHOR Youka)
set(PROJECT_DESCRIPTION "First renderer for subtitle format 'Substation Beta'")
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
message(STATUS "Project header: '${PROJECT_NAME}' by ${PROJECT_AUTHOR}, v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Enable tests (must be here for target generation)
enable_testing()
message(STATUS "Tests enabled")

# Process source
add_subdirectory(src)
message(STATUS "Finished source")

# Process documents
add_subdirectory(docs)
message(STATUS "Finished documents")