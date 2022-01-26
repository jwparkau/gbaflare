cmake_minimum_required(VERSION 3.21)

project(
	GBAFlare
	VERSION 0.1
	LANGUAGES CXX
)

#include(CheckIPOSupported)
#check_ipo_supported(RESULT supported OUTPUT error)
#
#if( supported )
#    message(STATUS "IPO / LTO enabled")
#    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#else()
#    message(STATUS "IPO / LTO not supported: <${error}>")
#endif()

add_executable(gbaflare "")
add_executable(gbaflare-release "")

if (USE_SDL2)
	message(STATUS "Using SDL2")
	add_compile_definitions(PLATFORM_USE_SDL2)
	find_package(SDL2 REQUIRED)
	target_link_libraries(gbaflare SDL2::SDL2)
	target_link_libraries(gbaflare-release SDL2::SDL2)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(gbaflare Threads::Threads)
target_link_libraries(gbaflare-release Threads::Threads)

add_subdirectory(src)
