target_compile_options(gbaflare PUBLIC -Wall -Wextra -Winline -O2 -DDEBUG)

set_target_properties(gbaflare
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

find_package(SDL2 REQUIRED)

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
)

target_include_directories(gbaflare PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gbaflare SDL2::SDL2)

target_compile_options(gbaflare-release PUBLIC -Wall -Wextra -Winline -O2)

set_target_properties(gbaflare-release
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

find_package(SDL2 REQUIRED)

target_sources(gbaflare-release PUBLIC
	release.cpp
)

target_include_directories(gbaflare-release PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gbaflare-release SDL2::SDL2)
