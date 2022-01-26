#include "main.h"

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

void main_loop(char **argv)
{
	set_initial_memory_state();

	load_bios_rom("../boot/gba_bios.bin");

	cartridge.filename = std::string(argv[1]);
	cartridge.save_file = cartridge.filename + ".flaresav";

	load_cartridge_rom();
	determine_save_type();

	switch (cartridge.save_type) {
		case SAVE_SRAM:
			load_sram();
			break;
		case SAVE_FLASH64:
		case SAVE_FLASH128:
			load_flash();
			break;
	}

	//cpu.fakeboot();
	cpu.fetch();
	cpu.fetch();

	next_event = 960;

	//platform_on_vblank();

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

	switch (cartridge.save_type) {
		case SAVE_SRAM:
			save_sram();
			break;
		case SAVE_FLASH64:
		case SAVE_FLASH128:
			save_flash();
			break;
	}
}
