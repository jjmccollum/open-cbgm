#Specify headers directory:
set(HEADERS_DIR ${CMAKE_SOURCE_DIR}/include)

# Add object source files:
set(SOURCES
	roaring.c
	pugixml.cpp
	sqlite3.c
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
add_library(objects OBJECT ${SOURCES})

# Point the build targets to their include directories:
target_include_directories(objects PRIVATE ${HEADERS_DIR})

# Find external libraries to link to:
find_library(PTHREAD_LIB NAMES pthread)
find_library(DL_LIB NAMES dl)

# Add all executable scripts to be generated:
add_executable(populate_db $<TARGET_OBJECTS:objects> populate_db.cpp)
add_executable(compare_witnesses $<TARGET_OBJECTS:objects> compare_witnesses.cpp)
add_executable(find_relatives $<TARGET_OBJECTS:objects> find_relatives.cpp)
add_executable(optimize_substemmata $<TARGET_OBJECTS:objects> optimize_substemmata.cpp)
add_executable(print_local_stemma $<TARGET_OBJECTS:objects> print_local_stemma.cpp)
add_executable(print_textual_flow $<TARGET_OBJECTS:objects> print_textual_flow.cpp)
add_executable(print_global_stemma $<TARGET_OBJECTS:objects> print_global_stemma.cpp)

# Point the build targets to their include directories:
target_include_directories(populate_db PRIVATE ${HEADERS_DIR})
target_include_directories(compare_witnesses PRIVATE ${HEADERS_DIR})
target_include_directories(find_relatives PRIVATE ${HEADERS_DIR})
target_include_directories(optimize_substemmata PRIVATE ${HEADERS_DIR})
target_include_directories(print_local_stemma PRIVATE ${HEADERS_DIR})
target_include_directories(print_textual_flow PRIVATE ${HEADERS_DIR})
target_include_directories(print_global_stemma PRIVATE ${HEADERS_DIR})

# Link the build targets to external libraries:
target_link_libraries(populate_db ${PTHREAD_LIB} ${DL_LIB})
target_link_libraries(compare_witnesses ${PTHREAD_LIB} ${DL_LIB})
target_link_libraries(find_relatives ${PTHREAD_LIB} ${DL_LIB})
target_link_libraries(optimize_substemmata ${PTHREAD_LIB} ${DL_LIB})
target_link_libraries(print_local_stemma ${PTHREAD_LIB} ${DL_LIB})
target_link_libraries(print_textual_flow ${PTHREAD_LIB} ${DL_LIB})
target_link_libraries(print_global_stemma ${PTHREAD_LIB} ${DL_LIB})

# Make sure the build targets are compiled with C++11:
target_compile_features(populate_db PRIVATE cxx_std_11)
target_compile_features(compare_witnesses PRIVATE cxx_std_11)
target_compile_features(find_relatives PRIVATE cxx_std_11)
target_compile_features(optimize_substemmata PRIVATE cxx_std_11)
target_compile_features(print_local_stemma PRIVATE cxx_std_11)
target_compile_features(print_textual_flow PRIVATE cxx_std_11)
target_compile_features(print_global_stemma PRIVATE cxx_std_11)