cmake_minimum_required(VERSION 3.1)
# Set the project name and version:
project(open-cbgm 
	VERSION 1.2.1
	DESCRIPTION "Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method"
	LANGUAGES C CXX)

# If the user does not specify a build type (e.g., "Debug"), then default to Release:
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Set the compilation warning flags:
set(WARNING_FLAGS "-Wall")
if(NOT MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Add an option for enabling or disabling unit tests:
option(ENABLE_TESTS "If OFF, disable unit tests" ON)

# Enable unit testing:
enable_testing()
include(CTest)

# Generate header files that contain the config variable for this make, 
# so we can access the right example files for unit testing:'
set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")
configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/test/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/config.h"
)

# Add the source code directory:
add_subdirectory(src)
# If we're testing, then add the test subdirectory, too:
if(ENABLE_TESTS)
    add_subdirectory(test)
endif()