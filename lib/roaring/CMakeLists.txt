project(roaring)
add_library(roaring roaring.c)
target_include_directories(roaring PUBLIC ${PROJECT_SOURCE_DIR})