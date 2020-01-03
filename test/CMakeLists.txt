#Specify test headers directory:
set(TEST_HEADERS_DIR ${CMAKE_SOURCE_DIR}/include)

# Add test header files:
set(TEST_HEADERS
	${TEST_HEADERS_DIR}/roaring.h
	${TEST_HEADERS_DIR}/roaring.hh
	${TEST_HEADERS_DIR}/pugiconfig.h
	${TEST_HEADERS_DIR}/pugixml.h
	${TEST_HEADERS_DIR}/local_stemma.h
	${TEST_HEADERS_DIR}/variation_unit.h
	${TEST_HEADERS_DIR}/apparatus.h
	${TEST_HEADERS_DIR}/set_cover_solver.h
	${TEST_HEADERS_DIR}/witness.h
	${TEST_HEADERS_DIR}/textual_flow.h
	${TEST_HEADERS_DIR}/global_stemma.h
	${TEST_HEADERS_DIR}/autotest.h
)

# Add all executable scripts to be generated:
add_executable(autotest $<TARGET_OBJECTS:objects> autotest.cpp ${TEST_HEADERS})

# Point the build targets to their include directories:
target_include_directories(autotest PRIVATE ${TEST_HEADERS_DIR})

# Make sure the build targets are compiled with C++11:
target_compile_features(autotest PRIVATE cxx_std_11)

# Register executables as tests:
add_test(NAME common_read_xml COMMAND autotest)
#add_test(NAME common_read_xml COMMAND autotest -t common_read_xml)
#add_test(NAME local_stemma_constructor_1 COMMAND autotest -t local_stemma_constructor_1)
#add_test(NAME local_stemma_constructor_2 COMMAND autotest -t local_stemma_constructor_2)
#add_test(NAME local_stemma_path_exists COMMAND autotest -t local_stemma_path_exists)
#add_test(NAME local_stemma_get_shortest_path_length COMMAND autotest -t local_stemma_get_shortest_path_length)
#add_test(NAME local_stemma_to_dot COMMAND autotest -t local_stemma_to_dot)