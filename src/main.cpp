#include "main.h"

#include <iostream>
#include <memory>

Arguments args;

void emulator_init(Arguments &args)
{
	set_initial_memory_state();

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

	cpu.flush_pipeline();
	cpu.sfetch();

	next_event = CYCLES_PER_SAMPLE;
	cpu_cycles = 0;
}

void main_loop()
{
	for (;;) {
		if (!emu_cnt.emulator_running) {
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

#define RESET_ARR(a, n)\
	for (int i = 0; i < (n); i++) {\
		a[i] = {};\
	}

#define ZERO_ARR(a) std::memset(a, 0, sizeof(a))

void emulator_reset()
{
	apu = {};
	RESET_ARR(fifos, 2);
	RESET_ARR(channel_states, NUM_PSG_CHANNELS);
	sweep_state = {};
	noise_state = {};
	wave_state = {};
	cpu = {};
	dma = {};
	flash = {};
	prefetch = {};
	ZERO_ARR(ewram_data);
	ZERO_ARR(iwram_data);
	ZERO_ARR(io_data);
	ZERO_ARR(palette_data);
	ZERO_ARR(vram_data);
	ZERO_ARR(oam_data);
	ZERO_ARR(wave_ram);
	last_bios_opcode = 0;
	prefetch_enabled = 0;
	ppu = {};
	next_event = 0;
	elapsed = 0;
	scheduler_flag = false;
	cpu_cycles = 0;
	timer = {};

	emulator_init(args);
}

void emulator_save_state(int n)
{

}

void emulator_load_state(int n)
{

}
