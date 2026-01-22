# PicoC/CMakeLists.txt
# Created by Robin Rowe 2024-04-09
# License New BSD License

cmake_minimum_required(VERSION 3.31)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(PicoC)
message("Configuring ${CMAKE_PROJECT_NAME}...")

if(NOT WIN32 AND NOT APPLE)
	link_libraries(rt pthread)
endif(NOT WIN32 AND NOT APPLE)

option(SUPPRESS "Enable suppression of code warnings" true)
if(SUPPRESS)
	if(WIN32)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4305 /wd4018 /wd26451 /wd6031 /wd4267 /wd4018)
	endif(WIN32)
endif(SUPPRESS)

option(UNISTD "Enable libunistd" true)
if(UNISTD)
	if(WIN32)
#		message("CMAKE_GENERATOR_PLATFORM = ${CMAKE_GENERATOR_PLATFORM}")
		set(LIBUNISTD_PATH /code/github/libunistd)
		include_directories(${LIBUNISTD_PATH}/unistd)
		set (UNISTD_LINK_DIRECTORIES ${LIBUNISTD_PATH}/build/${CMAKE_GENERATOR_PLATFORM}/Release)
		message(libunistd = ${UNISTD_LINK_DIRECTORIES}/libunistd.lib)
		link_directories(${UNISTD_LINK_DIRECTORIES})
		link_libraries(libunistd Ws2_32 Imm32)
	endif(WIN32)
endif(UNISTD)

option(DEBUGGER "Enable debugger" false)
if(DEBUGGER)
	add_definitions(-DDEBUGGER)
endif(DEBUGGER)

add_subdirectory(./cstdlib)
add_subdirectory(./platform)
#add_subdirectory(./tests)
#add_subdirectory(./tests/csmith)
#add_subdirectory(./tests/jpoirier)
file(STRINGS sources.cmake SOURCES)
add_executable(picoc ${SOURCES})
target_link_libraries(picoc cstdlib platform)
