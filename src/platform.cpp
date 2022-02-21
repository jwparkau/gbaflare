#include "platform.h"
#include "memory.h"
#include "main.h"

#include <string>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <cstdlib>
#include <filesystem>

const char *bios_filenames[] = {
	"gba_bios.bin",
	nullptr
};


const std::string prog_name = "gbaflare";

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
	std::memcpy(real_framebuffer, framebuffer, FRAMEBUFFER_SIZE * sizeof(*framebuffer));
	std::memcpy(real_audiobuffer, audiobuffer, AUDIOBUFFER_SIZE * sizeof(*audiobuffer));
	frame_drawn.release();

	std::chrono::duration<double> sec = std::chrono::steady_clock::now() - tick_start;
	if (print_fps) {
		fprintf(stderr, "fps: %f\n", 1 / sec.count());
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

	Arguments args;

	const char **fn;
	for (fn = bios_filenames; *fn; fn++) {
		fprintf(stderr, "trying bios file %s\n", *fn);
		std::ifstream f(*fn);
		if (f.good()) {
			break;
		}
	}
	if (*fn) {
		args.bios_filename = std::string(*fn);
	} else {
		int err = find_bios_file(args.bios_filename);
		if (err) {
			fprintf(stderr, "no bios file\n");
			return EXIT_FAILURE;
		}
	}

	int err = platform_init();
	if (err) {
		return EXIT_FAILURE;
	}

	if (argc >= 2) {
		args.cartridge_filename = std::string(argv[1]);
	} else {
		platform.wait_for_cartridge_file(args.cartridge_filename);
	}
	if (args.cartridge_filename.length() == 0) {
		fprintf(stderr, "no cartridge file\n");
		return EXIT_FAILURE;
	} else {
		fprintf(stderr, "using cartridge file %s\n", args.cartridge_filename.c_str());
	}

	emulator_init(args);

	std::thread emu_thread(main_loop);

	while (emulator_running) {
		frame_drawn.acquire();
		platform.render(real_framebuffer);
		platform.queue_audio();
		platform.handle_input();
		frame_rendered.release();
	}

	emu_thread.join();

	emulator_close();

	return 0;
}

int find_bios_file(std::string &s)
{
	char *t;
	std::string base_dir;

#ifdef __unix__
	t = std::getenv("XDG_DATA_HOME");
	if (t) {
		base_dir = std::string(t);
	} else {
		t = std::getenv("HOME");
		if (t) {
			base_dir = t + std::string("/.local/share");
		} else {
			return 1;
		}
	}
#elif defined (_WIN64) || defined (_WIN32)
	t = std::getenv("APPDATA");
	if (t) {
		base_dir = std::string(t);
	} else {
		return 1;
	}
#else
	return 1;
#endif
	std::string data_dir = base_dir + "/" + prog_name;
	std::filesystem::create_directory(data_dir);

	std::string filename = data_dir + "/" + bios_filenames[0];
	fprintf(stderr, "trying bios file %s\n", filename.c_str());
	std::ifstream f(filename);

	if (f.good()) {
		s = filename;
		return 0;
	} else {
		return 1;
	}
}

#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.cpp"
#endif
