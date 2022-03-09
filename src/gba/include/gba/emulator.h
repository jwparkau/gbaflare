#ifndef GBAFLARE_EMULATOR_H
#define GBAFLARE_EMULATOR_H

#include <common/types.h>
#include <gba/apu.h>

#include <string>

struct Arguments {
	std::string bios_filename;
	std::string cartridge_filename;
};

extern Arguments args;

struct Emulator {
	bool cartridge_loaded{};

	void init(Arguments &args);
	void run_one_frame();
	void close();
	void reset_memory();
	void reset();
	void quit();
};

extern Emulator emu;

struct SaveState {
	FIFO fifos[2];
	APU apu;
};

void emulator_save_state(int n);
void emulator_load_state(int n);

#endif
