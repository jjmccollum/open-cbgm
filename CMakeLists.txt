cmake_minimum_required(VERSION 3.1)
# Set the project name and version:
project(open-cbgm 
	VERSION 1.0
	DESCRIPTION "Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method"
	LANGUAGES C CXX)

# Add an option for enabling or disabling unit tests:
option(ENABLE_TESTS "If OFF, disable unit tests" ON)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

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