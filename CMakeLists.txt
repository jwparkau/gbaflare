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

if (USE_QT5)
	message(STATUS "Using QT5")
	add_compile_definitions(PLATFORM_USE_QT5)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	find_package(Qt5 COMPONENTS Widgets REQUIRED)
	find_package(Qt5 COMPONENTS Multimedia REQUIRED)
endif()

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED YES)
set (CMAKE_CXX_EXTENSIONS NO)

if (WINDOWS)
	set(QT_USE_MAIN true)
	add_executable(gbaflare WIN32 "")
else()
	add_executable(gbaflare "")
endif()

target_sources(gbaflare
	PUBLIC
	src/common/src/types.cpp
	src/gba/src/apu.cpp
	src/gba/src/arm.cpp
	src/gba/src/channel.cpp
	src/gba/src/cpu.cpp
	src/gba/src/dma.cpp
	src/gba/src/eeprom.cpp
	src/gba/src/flash.cpp
	src/gba/src/emulator.cpp
	src/gba/src/memory.cpp
	src/gba/src/ppu.cpp
	src/gba/src/scheduler.cpp
	src/gba/src/thumb.cpp
	src/gba/src/timer.cpp
	src/platform/src/common/platform.cpp
)
if (USE_QT5)
	target_sources(gbaflare
		PUBLIC
		src/platform/src/qt/main.cpp
		src/platform/src/qt/mainwindow.cpp
		src/platform/include/platform/qt/mainwindow.h
		src/platform/src/qt/mainwindow.ui
	)
endif()


target_include_directories(gbaflare
	PUBLIC
	src/common/include
	src/gba/include
	src/platform/include
)


if (USE_QT5)
	target_link_libraries(gbaflare Qt5::Widgets Qt5::Multimedia)
endif()

find_package(Threads REQUIRED)
target_link_libraries(gbaflare Threads::Threads)
