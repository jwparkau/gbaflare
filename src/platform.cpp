#include "platform.h"
#include "memory.h"

#include <stdexcept>
#include <iostream>

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
	platform.handle_input();
	io_write<u16>(IO_KEYINPUT, joypad_state);
	platform.render(framebuffer);
}

#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.cpp"
#endif
