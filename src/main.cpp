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

	cpu.fakeboot();
	cpu.fetch();
	cpu.fetch();

	Memory::write16(0x04000130, 0xFFFF);


	u64 tick_start = SDL_GetPerformanceCounter();
	u64 freq = SDL_GetPerformanceFrequency();

	u16 joypad_state = 0xFFFF;

	u32 t = 0;
	for (;;) {
		if (t % 1232 == 0) {
			platform.handle_input(joypad_state);
			Memory::write16(0x04000130, joypad_state);
		}
		if (!emulator_running) {
			break;
		}

		cpu.step();
		t += 1;

		if (t == 197120) {
			ppu.copy_framebuffer_mode4();
			platform.render(ppu.framebuffer);
			u16 x = Memory::read16(0x04000004);
			x |= 1;
			Memory::write16(0x04000004, x);
		}

		if (t == 280896) {
			u16 x = Memory::read16(0x04000004);
			x &= ~BITMASK(1);
			Memory::write16(0x04000004, x);
			t = 0;
			u64 ticks = SDL_GetPerformanceCounter() - tick_start;
			//printf("took %f ms\n", 1000.0 * ticks / freq);
			tick_start = SDL_GetPerformanceCounter();
		}
	}

	return 0;
}
