# cstdlib/CMakeLists.txt
# Created by Robin Rowe 2024-04-09
# License New BSD License

set (MODULE_NAME cstdlib)
message("Configuring ${MODULE_NAME} 9 source file(s)")
file(STRINGS sources.cmake SOURCES)
add_library(${MODULE_NAME} ${SOURCES})
link_libraries(${MODULE_NAME})