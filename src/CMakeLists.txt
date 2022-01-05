target_compile_options(gbaflare PUBLIC -Wall -Wextra -O2)

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
)

target_include_directories(gbaflare PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gbaflare SDL2::SDL2)
