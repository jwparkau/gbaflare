#include <iostream>

#include "cpu.h"
#include "memory.h"

int main(int argc, char **argv)
{
	fprintf(stderr, "GBAFlare - Gameboy Advance Emulator\n");

	if (argc != 2) {
		fprintf(stderr, "no cartridge file specified\n");
		exit(EXIT_FAILURE);
	}

	load_cartridge_rom(argv[1]);

	cpu.fakeboot();
	cpu.flush_pipeline();

	for (;;) {
		cpu.step();
	}

	return 0;
}
