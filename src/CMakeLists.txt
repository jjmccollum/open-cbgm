#Specify header directories:
set(OPEN_CBGM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

# Add object source files:
set(OPEN_CBGM_SOURCES
	local_stemma.cpp
	variation_unit.cpp
	apparatus.cpp
	set_cover_solver.cpp
	witness.cpp
	textual_flow.cpp
	global_stemma.cpp
	compare_witnesses_table.cpp
	find_relatives_table.cpp
	optimize_substemmata_table.cpp
)

# Build the object source files into a shared or static library, depending on the BUILD_SHARED_LIBS variable (default is OFF)
if(BUILD_SHARED_LIBS)
	MESSAGE("Building open-cbgm as shared library")
else()
	MESSAGE("Building open-cbgm as static library")
endif()
add_library(open-cbgm ${OPEN_CBGM_SOURCES})

# Point the library to its include directory:
target_include_directories(open-cbgm PUBLIC ${OPEN_CBGM_INCLUDE_DIR})

# Link it to its dependencies:
target_link_libraries(open-cbgm PUBLIC pugixml roaring)