cmake_minimum_required(VERSION 3.0)
# Set the project name and version:
project(open-cbgm 
	VERSION 1.3.1
	DESCRIPTION "Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method"
	LANGUAGES C CXX)

# Set the compilation warning flags:
set(WARNING_FLAGS "-Wall")
# Set optimization flags for each build type:
if(NOT MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Add an option for enabling or disabling unit tests:
option(ENABLE_TESTS "If OFF, disable unit tests" OFF)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
	set(ENABLE_TESTS ON)
	enable_testing()
	include(CTest)
endif()

# Set build options and locations if they have not been set already:
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Path to build archive directory")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Path to build library directory")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin CACHE PATH "Path to build runtime directory")

# Generate header files that contain the config variable for this make, 
# so we can access the right example files for unit testing:'
set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")
configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/test/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/config.h"
)

# Add the source code directory:
add_subdirectory(src)
# Add the external library directories:
add_subdirectory(lib/pugixml)
add_subdirectory(lib/roaring)
# If we're testing, then add the test subdirectory, too:
if(ENABLE_TESTS)
    add_subdirectory(test)
endif()