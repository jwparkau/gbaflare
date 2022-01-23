#include <iostream>
#include <memory>

#include "types.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "platform.h"
#include "timer.h"
#include "scheduler.h"
#include "dma.h"

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

	cartridge.filename = std::string(argv[1]);
	cartridge.save_file = cartridge.filename + ".flaresav";

	load_cartridge_rom();
	determine_save_type();
	if (cartridge.save_type == SAVE_SRAM) {
		load_sram();
	}

	int err = platform_init();
	if (err) {
		return EXIT_FAILURE;
	}

	set_initial_memory_state();

	//cpu.fakeboot();
	cpu.fetch();
	cpu.fetch();

	next_event = 960;

	platform_on_vblank();

	for (;;) {
		if (!emulator_running) {
			break;
		}

		while (cpu_cycles < next_event) {
			if (dma.dma_active) {
				dma.step();
			} else {
				cpu.step();
			}
		}

		start_event_processing();

		timer.step();
		ppu.step();

		end_event_processing();
	}

	if (cartridge.save_type == SAVE_SRAM) {
		save_sram();
	}

	return 0;
}
