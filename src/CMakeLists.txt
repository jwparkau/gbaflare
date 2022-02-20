target_compile_options(gbaflare PUBLIC)

set_target_properties(gbaflare
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_sources(gbaflare PUBLIC
	main.cpp
	cpu.cpp
	memory.cpp
	types.cpp
	arm.cpp
	ops.cpp
	thumb.cpp
	ppu.cpp
	platform.cpp
	timer.cpp
	scheduler.cpp
	dma.cpp
	flash.cpp
	apu.cpp
	channel.cpp
)

target_include_directories(gbaflare PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)


target_compile_options(gbaflare-release PUBLIC)

set_target_properties(gbaflare-release
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_sources(gbaflare-release PUBLIC
	release.cpp
)

target_include_directories(gbaflare-release PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
