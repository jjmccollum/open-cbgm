# Specify test headers directory:
set(TEST_HEADERS_DIR ${PROJECT_SOURCE_DIR}/include)

# Add all executable scripts to be generated:
add_executable(autotest autotest.cpp)

# Point the build targets to their include directories:
target_include_directories(autotest PRIVATE ${TEST_HEADERS_DIR})

# Link the build targets to external libraries:
target_link_libraries(autotest PUBLIC roaring pugixml open-cbgm)

# Make sure the build targets are compiled with C++11:
target_compile_features(autotest PRIVATE cxx_std_11)

# Register executables as tests:
add_test(NAME common_read_xml COMMAND autotest -t common_read_xml)
add_test(NAME local_stemma_constructor_1 COMMAND autotest -t local_stemma_constructor_1)
add_test(NAME local_stemma_constructor_2 COMMAND autotest -t local_stemma_constructor_2)
add_test(NAME local_stemma_path_exists COMMAND autotest -t local_stemma_path_exists)
add_test(NAME local_stemma_get_shortest_path_length COMMAND autotest -t local_stemma_get_shortest_path_length)
add_test(NAME local_stemma_to_dot COMMAND autotest -t local_stemma_to_dot)
add_test(NAME variation_unit_constructor_1 COMMAND autotest -t variation_unit_constructor_1)
add_test(NAME variation_unit_constructor_2 COMMAND autotest -t variation_unit_constructor_2)
add_test(NAME variation_unit_constructor_3 COMMAND autotest -t variation_unit_constructor_3)
add_test(NAME variation_unit_constructor_4 COMMAND autotest -t variation_unit_constructor_4)
add_test(NAME apparatus_constructor COMMAND autotest -t apparatus_constructor)
add_test(NAME apparatus_get_extant_passages_for_witness COMMAND autotest -t apparatus_get_extant_passages_for_witness)
add_test(NAME set_cover_solver_constructor COMMAND autotest -t set_cover_solver_constructor)
add_test(NAME set_cover_solver_get_unique_rows COMMAND autotest -t set_cover_solver_get_unique_rows)
add_test(NAME set_cover_solver_get_trivial_solution COMMAND autotest -t set_cover_solver_get_trivial_solution)
add_test(NAME set_cover_solver_get_greedy_solution COMMAND autotest -t set_cover_solver_get_greedy_solution)
add_test(NAME witness_constructor_1 COMMAND autotest -t witness_constructor_1)
add_test(NAME witness_constructor_2 COMMAND autotest -t witness_constructor_2)
add_test(NAME witness_get_genealogical_comparison_for_witness COMMAND autotest -t witness_get_genealogical_comparison_for_witness)
add_test(NAME witness_set_potential_ancestor_ids COMMAND autotest -t witness_set_potential_ancestor_ids)
add_test(NAME witness_set_global_stemma_ancestor_ids COMMAND autotest -t witness_set_global_stemma_ancestor_ids)
add_test(NAME textual_flow_constructor COMMAND autotest -t textual_flow_constructor)
add_test(NAME textual_flow_textual_flow_to_dot COMMAND autotest -t textual_flow_textual_flow_to_dot)
add_test(NAME textual_flow_coherence_in_attestations_to_dot COMMAND autotest -t textual_flow_coherence_in_attestations_to_dot)
add_test(NAME textual_flow_coherence_in_variant_passages_to_dot COMMAND autotest -t textual_flow_coherence_in_variant_passages_to_dot)
add_test(NAME global_stemma_constructor COMMAND autotest -t global_stemma_constructor)
add_test(NAME global_stemma_to_dot COMMAND autotest -t global_stemma_to_dot)
