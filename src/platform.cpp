#include "platform.h"
#include "memory.h"
#include "emulator.h"

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

EmulatorControl emu_cnt;

u16 framebuffer[FRAMEBUFFER_SIZE];
s16 audiobuffer[AUDIOBUFFER_SIZE];
std::atomic_uint32_t audio_buffer_index;

void platform_on_vblank()
{
	if (emu_cnt.save_state) {
		emulator_save_state(emu_cnt.save_state);
		emu_cnt.save_state = 0;
	}

	if (emu_cnt.load_state) {
		emulator_load_state(emu_cnt.load_state);
		emu_cnt.load_state = 0;
	}

	if (emu_cnt.do_reset) {
		emu.reset();
		emu_cnt.do_reset = false;
	}

	if (emu_cnt.do_close) {
		emu.close();
		emu_cnt.do_close = false;
	}

	io_write<u16>(IO_KEYINPUT, emu_cnt.joypad_state);
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
