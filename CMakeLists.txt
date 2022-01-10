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

add_subdirectory(src)
