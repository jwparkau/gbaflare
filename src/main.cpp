#include <iostream>

#include <SDL2/SDL.h>

#include "types.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

static bool quit = false;

struct Platform {
	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};

	int width = 240;
	int height = 160;
	int scale_factor = 8;

	void init();

	void copy_framebuffer();
	void render(u16 *pixels);
	void handle_input();
} platform;

void Platform::init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		throw std::runtime_error("SDL init failed");
	}

	window = SDL_CreateWindow("Gameboy Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width*scale_factor, height*scale_factor, SDL_WINDOW_SHOWN);
	if (!window) {
		throw std::runtime_error("SDL window could not be created");
	}

	SDL_SetWindowResizable(window, SDL_FALSE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		throw std::runtime_error("SDL renderer could not be created");
	}

	if (SDL_RenderSetLogicalSize(renderer, width, height) < 0) {
		throw std::runtime_error("SDL logical size could not be set");
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!texture) {
		throw std::runtime_error("SDL texture could not be created");
	}
}

void Platform::copy_framebuffer()
{
	for (int i = 0; i < 240 * 160; i++) {
		u32 offset = Memory::read8(VRAM_START + i);
		ppu.framebuffer[i] = Memory::read16(offset * 2 + 0x05000000);
	}
}

void Platform::render(u16 *pixels)
{
	/*
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_Rect r = {0, 0, width, height};
	SDL_RenderFillRect(renderer, &r);
	*/

	SDL_UpdateTexture(texture, NULL, pixels, width * sizeof *pixels);

	SDL_RenderCopy(renderer, texture, NULL, NULL);

	SDL_RenderPresent(renderer);
}

void Platform::handle_input()
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			quit = true;
			break;
		}
	}

}

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

	platform.init();

	cpu.fakeboot();
	cpu.fetch();
	cpu.fetch();

	Memory::write16(0x04000130, 0xFFFF);


	u64 tick_start = SDL_GetPerformanceCounter();
	u64 freq = SDL_GetPerformanceFrequency();

	u32 t = 0;
	for (;;) {
		if (t % 1232 == 0) {
			platform.handle_input();
		}
		if (quit) {
			break;
		}

		cpu.step();
		t += 1;

		if (t == 197120) {
			platform.copy_framebuffer();
			platform.render(ppu.framebuffer);
			// TODO: UNDEFINED
			u16 x = Memory::read16(0x04000004);
			x |= 1;
			Memory::write16(0x04000004, x);
			//platform.render((u16 *)vram_data);
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
