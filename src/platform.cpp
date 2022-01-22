#include "platform.h"
#include "memory.h"

#include <stdexcept>
#include <iostream>
#include <chrono>

bool emulator_running;
u16 joypad_state = 0xFFFF;
bool throttle_enabled = true;
bool print_fps;

Platform platform;

u16 framebuffer[FRAMEBUFFER_SIZE];

int platform_init()
{
	return platform.init();
}

void platform_on_vblank()
{
	static auto tick_start = std::chrono::steady_clock::now();
	static std::chrono::duration<double> frame_duration(1.0 / 60);

	platform.handle_input();
	io_write<u16>(IO_KEYINPUT, joypad_state);
	platform.render(framebuffer);

	std::chrono::duration<double> sec = std::chrono::steady_clock::now() - tick_start;
	if (print_fps) {
		printf("fps: %f\n", 1 / sec.count());
	}

	if (throttle_enabled) {
		while (std::chrono::steady_clock::now() - tick_start < frame_duration)
			;
	}

	tick_start = std::chrono::steady_clock::now();
}

#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.cpp"
#endif
