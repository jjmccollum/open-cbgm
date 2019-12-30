#Specify headers directory:
set(HEADERS_DIR ${CMAKE_SOURCE_DIR}/include)

# Add header files:
set(HEADERS
	${HEADERS_DIR}/cxxopts.h
	${HEADERS_DIR}/roaring.h
	${HEADERS_DIR}/roaring.hh
	${HEADERS_DIR}/pugiconfig.h
	${HEADERS_DIR}/pugixml.h
	${HEADERS_DIR}/local_stemma.h
	${HEADERS_DIR}/variation_unit.h
	${HEADERS_DIR}/apparatus.h
	${HEADERS_DIR}/set_cover_solver.h
	${HEADERS_DIR}/witness.h
	${HEADERS_DIR}/textual_flow.h
	${HEADERS_DIR}/global_stemma.h
)

# Add object source files:
set(SOURCES 
	roaring.c
	pugixml.cpp
	local_stemma.cpp
	variation_unit.cpp
	apparatus.cpp
	set_cover_solver.cpp
	witness.cpp
	textual_flow.cpp
	global_stemma.cpp
)

# Combine the object source files into an object library:
# TODO: We could allow the user to specify on the command line if a STATIC or SHARED library is desired instead,
# to facilitate incorporating the core modules into their own projects.
add_library(objects OBJECT ${SOURCES} ${HEADERS})

# Point the build targets to their include directories:
target_include_directories(objects PRIVATE ${HEADERS_DIR})

# Add all executable scripts to be generated:
add_executable(compare_witnesses $<TARGET_OBJECTS:objects> compare_witnesses.cpp )
add_executable(find_relatives $<TARGET_OBJECTS:objects> find_relatives.cpp)
add_executable(optimize_substemmata $<TARGET_OBJECTS:objects> optimize_substemmata.cpp)
add_executable(print_graphs $<TARGET_OBJECTS:objects> print_graphs.cpp)

# Point the build targets to their include directories:
target_include_directories(compare_witnesses PRIVATE ${HEADERS_DIR})
target_include_directories(find_relatives PRIVATE ${HEADERS_DIR})
target_include_directories(optimize_substemmata PRIVATE ${HEADERS_DIR})
target_include_directories(print_graphs PRIVATE ${HEADERS_DIR})

# Make sure the build targets are compiled with C++11:
target_compile_features(compare_witnesses PRIVATE cxx_std_11)
target_compile_features(find_relatives PRIVATE cxx_std_11)
target_compile_features(optimize_substemmata PRIVATE cxx_std_11)
target_compile_features(print_graphs PRIVATE cxx_std_11)