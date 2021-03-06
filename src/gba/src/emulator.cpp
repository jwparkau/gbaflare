#include <gba/emulator.h>
#include <gba/apu.h>
#include <gba/cpu.h>
#include <gba/dma.h>
#include <gba/scheduler.h>
#include <gba/memory.h>

#include <iostream>
#include <memory>

Arguments args;
Emulator emu;

void Emulator::init(Arguments &args)
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
		case SAVE_EEPROM_UNKNOWN:
		case SAVE_EEPROM4:
		case SAVE_EEPROM64:
			load_eeprom();
			break;
	}

	cartridge_loaded = true;

	cpu.flush_pipeline();
	cpu.sfetch();

	next_event = CYCLES_PER_SAMPLE;
	cpu_cycles = 0;
}

void Emulator::run_one_frame()
{
	for (;;) {
		while (cpu_cycles < next_event) {
			if (dma.dma_active) {
				dma.step();
			} else {
				cpu.step();
			}
		}

		start_event_processing();

		dma.update();
		ppu.step();
		apu.channel_step();
		apu.step();
		timer.step();

		end_event_processing();

		if (ppu.vblank) {
			ppu.vblank = false;
			ppu.on_vblank();
			dma.on_vblank();
			apu.audio_buffer_index = 0;
			io_write<u16>(IO_KEYINPUT, emu_cnt.joypad_state);
			break;
		}
	}
}

void Emulator::close()
{
	if (cartridge_loaded) {
		switch (cartridge.save_type) {
			case SAVE_SRAM:
				save_sram();
				break;
			case SAVE_FLASH64:
			case SAVE_FLASH128:
				save_flash();
				break;
			case SAVE_EEPROM4:
			case SAVE_EEPROM64:
				save_eeprom();
				break;
		}
	}

	reset_memory();

	cartridge_loaded = false;
}

void Emulator::quit()
{
	close();
}

void Emulator::reset_memory()
{
	apu = {};
	RESET_ARR(fifos, 2);
	RESET_ARR(channel_states, NUM_PSG_CHANNELS);
	sweep_state = {};
	noise_state = {};
	wave_state = {};
	cpu = {};
	dma = {};
	flash.reset();
	eeprom.reset();
	prefetch = {};
	ZERO_ARR(ewram_data);
	ZERO_ARR(iwram_data);
	ZERO_ARR(io_data);
	ZERO_ARR(palette_data);
	ZERO_ARR(vram_data);
	ZERO_ARR(oam_data);
	ZERO_ARR(cartridge_data);
	ZERO_ARR(sram_data);
	ZERO_ARR(wave_ram);
	last_bios_opcode = 0;
	prefetch_enabled = 0;
	ppu.reset();
	next_event = 0;
	elapsed = 0;
	scheduler_flag = false;
	cpu_cycles = 0;
	timer = {};
}

void Emulator::reset()
{
	close();
	init(args);
}

void emulator_save_state(int n)
{

}

void emulator_load_state(int n)
{

}
