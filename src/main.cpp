#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

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
	load_cartridge_rom(argv[1]);

	int err = platform_init();
	if (err) {
		return EXIT_FAILURE;
	}

	set_initial_memory_state();

	//cpu.fakeboot();
	cpu.fetch();
	cpu.fetch();

	next_event = 960;

	auto tick_start = std::chrono::steady_clock::now();
	std::chrono::duration<double> frame_duration(1.0 / 60);

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

		if (vblank_flag == 1) {
			std::chrono::duration<double> sec = std::chrono::steady_clock::now() - tick_start;
			if (print_fps) {
				printf("fps: %f\n", 1 / sec.count());
			}

			if (throttle_enabled) {
				while (std::chrono::steady_clock::now() - tick_start < frame_duration)
					;
			}

			tick_start = std::chrono::steady_clock::now();
			vblank_flag = 0;
		}
	}

	return 0;
}
