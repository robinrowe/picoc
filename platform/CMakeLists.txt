# platform/CMakeLists.txt
# Created by Robin Rowe 2024-04-09
# License New BSD License

set (MODULE_NAME platform)
message("Configuring ${MODULE_NAME} 4 source file(s)")
#file(STRINGS sources.cmake SOURCES)
if(WIN32)
	set(SOURCES library_msvc.c platform_msvc.c)
else(WIN32)
	set(SOURCES library_unix.c platform_unix.c)
endif(WIN32)
add_library(${MODULE_NAME} ${SOURCES})
