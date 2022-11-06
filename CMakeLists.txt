cmake_minimum_required(VERSION 3.4)
# Set the project name and version:
project(open-cbgm 
	VERSION 1.5.0
	DESCRIPTION "Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method"
	LANGUAGES C CXX)

include(CTest)

# Set the compilation warning flags:
set(WARNING_FLAGS "-Wall")
# Set optimization flags for each build type:
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

# Set build options and locations if they have not been set already:
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Path to build archive directory")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Path to build library directory")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin CACHE PATH "Path to build runtime directory")

# Disable testing unless the BUILD_TESTS option is set:
option(BUILD_TESTS "Build open-cbgm unit tests" OFF)

# Generate header files that contain the config variable for this make, 
# so we can access the right example files for unit testing:'
set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")
configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/test/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/config.h"
)

# Add the external library directories:
option(CXXOPTS_BUILD_EXAMPLES "Set to ON to build examples" OFF)
option(CXXOPTS_BUILD_TESTS "Set to ON to build tests" OFF)
add_subdirectory(lib/cxxopts)
add_subdirectory(lib/pugixml)
option(ENABLE_ROARING_TESTS "If OFF, disable unit tests altogether" OFF)
option(ROARING_BUILD_STATIC "Build Roaring as a static library" ON)
if (BUILD_SHARED_LIBS)
	set(ROARING_BUILD_STATIC OFF)
endif()
add_subdirectory(lib/roaring)
# Add the source code directories:
add_subdirectory(src)
# Optionally add the test directory:
if (BUILD_TESTS)
	add_subdirectory(test)
endif()
