# tests_jpoirier/CMakeLists.txt
# Created by Robin Rowe 2024-04-09
# License New BSD License

set (MODULE_NAME tests_jpoirier)
message("Configuring ${MODULE_NAME} 1 source file(s)")
file(STRINGS sources.cmake SOURCES)
add_executable(${MODULE_NAME} ${SOURCES})
