#ifndef GBAFLARE_MAIN_H
#define GBAFLARE_MAIN_H

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

void emulator_init(Arguments &args);
void main_loop();
void emulator_close();
void emulator_reset();

struct SaveState {
	FIFO fifos[2];
	APU apu;
};

void emulator_save_state(int n);
void emulator_load_state(int n);

#endif
