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
	if (request_load_bios) {
		request_load_bios = false;

		if (emulator_state == EMULATION_NOBIOS || emulator_state == EMULATION_STOPPED) {
			shared.lock.lock();
			args.bios_filename = shared.bios_filename;
			shared.lock.unlock();
			load_bios_rom(args.bios_filename);

			emulator_state = EMULATION_STOPPED;
		}
	}

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

std::string get_data_dir()
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
		}
	}
#elif defined (_WIN64) || defined (_WIN32)
	t = std::getenv("APPDATA");
	if (t) {
		base_dir = std::string(t);
	}
#else
	;
#endif
	std::string data_dir;

	if (base_dir.length() > 0) {
		data_dir = base_dir + "/" + prog_name;
	}
	return data_dir;
}

std::string get_default_bios_path()
{
	std::string data_dir = get_data_dir();
	if (data_dir.length() > 0) {
		return data_dir + "/" + bios_filenames[0];
	} else {
		return {""};
	}
}

std::string get_default_bios_path(const std::string &s)
{
	return s + "/" + bios_filenames[0];
}

int find_bios_file(std::string &s)
{
	std::string data_dir = get_data_dir();
	if (data_dir.length() == 0) {
		return 1;
	}

	std::filesystem::create_directory(data_dir);

	std::string filename = get_default_bios_path(data_dir);
	std::ifstream f(filename);

	if (f.good()) {
		s = filename;
		return 0;
	} else {
		return 1;
	}
}

void copy_bios_file(std::string &s)
{
	std::string data_dir = get_data_dir();
	if (data_dir.length() == 0) {
		return;
	}

	std::filesystem::create_directory(data_dir);

	std::string filename = get_default_bios_path(data_dir);
	bool copied = std::filesystem::copy_file(s, filename);
	if (copied) {
		fprintf(stderr, "copied bios file %s to %s\n", s.c_str(), filename.c_str());
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

