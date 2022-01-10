#include <SDL2/SDL.h>

#include <iostream>
#include <memory>

#include "types.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "platform.h"

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

	load_bios_rom("../boot/gba_bios.bin");
	load_cartridge_rom(argv[1]);

	int err = platform.init();
	if (err) {
		return EXIT_FAILURE;
	}

	set_initial_memory_state();

	cpu.fakeboot();
	cpu.fetch();
	cpu.fetch();

	u64 tick_start = SDL_GetPerformanceCounter();
	u64 freq = SDL_GetPerformanceFrequency();

	u16 joypad_state = 0xFFFF;

	u32 t = 0;
	for (;;) {
		if (!emulator_running) {
			break;
		}

		cpu.step();
		ppu.step();
		t += 1;

		if (t == 197120) {
			platform.handle_input(joypad_state);
			io_write<u16>(IO_KEYINPUT, joypad_state);
		}

		if (t == 280896) {
			t = 0;
			u64 ticks = SDL_GetPerformanceCounter() - tick_start;
			//printf("took %f ms\n", 1000.0 * ticks / freq);
			tick_start = SDL_GetPerformanceCounter();
		}
	}

	return 0;
}
