#Specify headers directory:
set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/include)

# Add object source files:
set(SOURCES
	local_stemma.cpp
	variation_unit.cpp
	apparatus.cpp
	set_cover_solver.cpp
	witness.cpp
	textual_flow.cpp
	global_stemma.cpp
)

# Combine the object source files into a static library:
# TODO: We could allow the user to specify on the command line if a STATIC or SHARED library is desired instead,
# to facilitate incorporating the core modules into their own projects.
add_library(roaring STATIC roaring.c)
add_library(pugixml STATIC pugixml.cpp)
add_library(open-cbgm STATIC ${SOURCES})

# Point the libraries to their include directories:
target_include_directories(roaring PRIVATE ${HEADERS_DIR})
target_include_directories(pugixml PRIVATE ${HEADERS_DIR})
target_include_directories(open-cbgm PRIVATE ${HEADERS_DIR})