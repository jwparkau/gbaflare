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
	u64 ff = 0;
	for (;;) {
		if (!emulator_running) {
			break;
		}

		cpu.step();
		t += 1;
		ff += 1;

		if (t == 197120) {
			platform.handle_input(joypad_state);
			writearr<u16>(io_data, IO_KEYINPUT - IO_START, joypad_state);
			ppu.copy_framebuffer_mode4();
			platform.render(ppu.framebuffer);
			u16 x = readarr<u16>(io_data, IO_DISPSTAT - IO_START);
			x |= 1;
			writearr<u16>(io_data, IO_DISPSTAT - IO_START, x);
		}

		if (t == 280896) {
			u16 x = readarr<u16>(io_data, IO_DISPSTAT - IO_START);
			x &= ~BITMASK(1);
			writearr<u16>(io_data, IO_DISPSTAT - IO_START, x);
			t = 0;
			u64 ticks = SDL_GetPerformanceCounter() - tick_start;
			printf("took %f ms\n", 1000.0 * ticks / freq);
			tick_start = SDL_GetPerformanceCounter();
		}

		if (ff == 200000000) {
			return 0;
		}
	}

	return 0;
}
