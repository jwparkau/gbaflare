#include <iostream>

#include "cpu.h"

int main(int argc, char **argv)
{
	fprintf(stderr, "GBAFlare - Gameboy Advance Emulator\n");

	for (;;) {
		cpu.step();
	}

	return 0;
}
