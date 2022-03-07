#ifndef GBAFLARE_EMULATOR_H
#define GBAFLARE_EMULATOR_H

#include "types.h"
#include "apu.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "platform.h"
#include "timer.h"
#include "scheduler.h"
#include "dma.h"

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
};

extern Emulator emu;

struct SaveState {
	FIFO fifos[2];
	APU apu;
};

void emulator_save_state(int n);
void emulator_load_state(int n);

#endif
