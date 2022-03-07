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
		src/gba/src/release.cpp
	)
else()
	target_sources(gbaflare
		PUBLIC
		src/common/src/types.cpp
		src/gba/src/apu.cpp
		src/gba/src/arm.cpp
		src/gba/src/channel.cpp
		src/gba/src/cpu.cpp
		src/gba/src/dma.cpp
		src/gba/src/flash.cpp
		src/gba/src/emulator.cpp
		src/gba/src/memory.cpp
		src/gba/src/ppu.cpp
		src/gba/src/scheduler.cpp
		src/gba/src/thumb.cpp
		src/gba/src/timer.cpp
		src/platform/src/common/platform.cpp
	)
	if (USE_SDL2)
		target_sources(gbaflare
			PUBLIC
			src/platform/src/sdl2/platform_sdl.cpp
		)
	endif()
endif()


target_include_directories(gbaflare
	PUBLIC
	src/common/include
	src/gba/include
	src/platform/include
)

if (USE_SDL2)
	target_link_libraries(gbaflare ${SDL2_LIBRARIES})
endif()

find_package(Threads REQUIRED)
target_link_libraries(gbaflare Threads::Threads)
