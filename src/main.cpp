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

void emulator_init(Arguments &args)
{
	set_initial_memory_state();

	load_bios_rom(args.bios_filename);

	cartridge.filename = args.cartridge_filename;
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
}

void main_loop()
{
	cpu.flush_pipeline();
	cpu.sfetch();

	next_event = CYCLES_PER_SAMPLE;
	cpu_cycles = 0;

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

		ppu.step();
		timer.step();
		apu.channel_step();
		apu.step();

		end_event_processing();
	}
}

void emulator_close()
{
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
