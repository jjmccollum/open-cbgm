project(pugixml)
add_library(pugixml pugixml.cpp)
# Point the library to its include directory:
target_include_directories(pugixml PUBLIC ${PROJECT_SOURCE_DIR})