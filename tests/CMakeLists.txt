# tests/CMakeLists.txt
# Created by Robin Rowe 2024-04-09
# License New BSD License

set (MODULE_NAME tests)
message("Configuring ${MODULE_NAME} 70 source file(s)")
file(STRINGS sources.cmake SOURCES)
add_executable(${MODULE_NAME} ${SOURCES})
