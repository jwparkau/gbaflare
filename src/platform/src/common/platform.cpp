#include <platform/common/platform.h>
#include <gba/memory.h>
#include <gba/emulator.h>

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
SharedState shared;

u16 framebuffer[FRAMEBUFFER_SIZE];
s16 audiobuffer[AUDIOBUFFER_SIZE];

void EmulatorControl::process_events()
{
	if (request_open) {
		request_open = false;

		if (emulator_state == EMULATION_STOPPED) {
			emu.reset_memory();
			shared.lock.lock();
			args.cartridge_filename = shared.cartridge_filename;
			shared.lock.unlock();
			emu.init(args);

			emulator_state = EMULATION_RUNNING;
		}
	}

	if (request_close) {
		request_close = false;

		if (emulator_state == EMULATION_PAUSED || emulator_state == EMULATION_RUNNING) {
			emu.close();
			on_close();
			emulator_state = EMULATION_STOPPED;
		}
	}

	if (request_reset) {
		request_reset = false;

		if (emulator_state == EMULATION_PAUSED || emulator_state == EMULATION_RUNNING) {
			emu.reset();

			emulator_state = EMULATION_RUNNING;
		}
	}

	if (request_pause) {
		request_pause = false;

		if (emulator_state == EMULATION_PAUSED) {
			emulator_state = EMULATION_RUNNING;
		} else if (emulator_state == EMULATION_RUNNING) {
			on_pause();
			emulator_state = EMULATION_PAUSED;
		}
	}
}

void EmulatorControl::on_close()
{
	throttle_enabled = true;
	print_fps = false;
}

void EmulatorControl::on_pause()
{
	print_fps = false;
}

void EmulatorControl::toggle_throttle()
{
	if (emulator_state == EMULATION_RUNNING) {
		throttle_enabled = !throttle_enabled;
	}
}

void EmulatorControl::toggle_print_fps()
{
	if (emulator_state == EMULATION_PAUSED || emulator_state == EMULATION_RUNNING) {
		print_fps = !print_fps;
	}
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

void update_joypad(joypad_buttons button, bool down)
{
	if (button != NOT_MAPPED) {
		if (down) {
			emu_cnt.joypad_state &= ~BIT(button);
		} else {
			emu_cnt.joypad_state |= BIT(button);
		}
	}
}

