cmake_minimum_required(VERSION 3.21)

project(
	GBAFlare
	VERSION 0.1
	LANGUAGES CXX
)

if ( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
	add_compile_options(-Wall -Wextra -Winline)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if( supported )
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

if (USE_SDL2)
	message(STATUS "Using SDL2")
	add_compile_definitions(PLATFORM_USE_SDL2)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
endif()

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED YES)
set (CMAKE_CXX_EXTENSIONS NO)

add_executable(gbaflare "")

if (SINGLE_SOURCE)
	target_sources(gbaflare
		PUBLIC
		src/release.cpp)
else()
	target_sources(gbaflare
		PUBLIC
		src/apu.cpp
		src/arm.cpp
		src/channel.cpp
		src/cpu.cpp
		src/dma.cpp
		src/flash.cpp
		src/emulator.cpp
		src/memory.cpp
		src/ops.cpp
		src/platform.cpp
		src/ppu.cpp
		src/scheduler.cpp
		src/thumb.cpp
		src/timer.cpp
		src/types.cpp
	)
endif()

target_include_directories(gbaflare PUBLIC src)

if (USE_SDL2)
	target_link_libraries(gbaflare ${SDL2_LIBRARIES})
endif()

find_package(Threads REQUIRED)
target_link_libraries(gbaflare Threads::Threads)
