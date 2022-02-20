#include "platform.h"
#include "memory.h"
#include "main.h"

#include <string>
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

s16 audiobuffer[AUDIOBUFFER_SIZE];
s16 real_audiobuffer[AUDIOBUFFER_SIZE];
std::atomic_uint32_t audio_buffer_index;

int platform_init()
{
	return platform.init();
}

void platform_on_vblank()
{
	static auto tick_start = std::chrono::steady_clock::now();
	static std::chrono::duration<double> frame_duration(1.0 / FPS);

	frame_rendered.acquire();
	f_lock.lock();
	std::memcpy(real_framebuffer, framebuffer, FRAMEBUFFER_SIZE * sizeof(*framebuffer));
	std::memcpy(real_audiobuffer, audiobuffer, AUDIOBUFFER_SIZE * sizeof(*audiobuffer));
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

	int err = platform_init();
	if (err) {
		return EXIT_FAILURE;
	}

	std::string cartridge_filename;
	if (argc >= 2) {
		cartridge_filename = std::string(argv[1]);
	} else {
		platform.wait_for_cartridge_file(cartridge_filename);
	}

	if (cartridge_filename.length() == 0) {
		fprintf(stderr, "no cartridge file\n");
		return EXIT_FAILURE;
	}

	emulator_init(cartridge_filename);

	std::thread emu_thread(main_loop);

	while (emulator_running) {
		frame_drawn.acquire();
		f_lock.lock();
		platform.render(real_framebuffer);
		platform.queue_audio();
		platform.handle_input();
		f_lock.unlock();
		frame_rendered.release();
	}

	emu_thread.join();

	emulator_close();

	return 0;
}

#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.cpp"
#endif
