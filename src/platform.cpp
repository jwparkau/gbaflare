#include "platform.h"
#include "memory.h"
#include "main.h"

#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>

std::atomic_bool emulator_running;
std::atomic_bool throttle_enabled = true;
std::atomic_bool print_fps;
std::atomic_uint16_t joypad_state = 0xFFFF;

Platform platform;

u16 framebuffer[FRAMEBUFFER_SIZE];
u16 real_framebuffer[FRAMEBUFFER_SIZE];
std::binary_semaphore frame_rendered{1}, frame_drawn{0};
std::mutex f_lock;

int platform_init()
{
	return platform.init();
}

void platform_on_vblank()
{
	static auto tick_start = std::chrono::steady_clock::now();
	static std::chrono::duration<double> frame_duration(1.0 / 60);

	frame_rendered.acquire();
	f_lock.lock();
	std::memcpy(real_framebuffer, framebuffer, FRAMEBUFFER_SIZE * sizeof(*framebuffer));
	f_lock.unlock();
	frame_drawn.release();

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

int main(int argc, char **argv)
{
	fprintf(stderr, "GBAFlare - Gameboy Advance Emulator\n");

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "no cartridge file specified\n");
		exit(EXIT_FAILURE);
	}

	if (argc == 3) {
		debug = true;
	}

	int err = platform_init();
	if (err) {
		return EXIT_FAILURE;
	}

	std::thread emu_thread(main_loop, argv);

	while (emulator_running) {
		frame_drawn.acquire();
		f_lock.lock();
		platform.render(real_framebuffer);
		platform.handle_input();
		f_lock.unlock();
		frame_rendered.release();
	}

	emu_thread.join();

	return 0;
}

#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.cpp"
#endif
