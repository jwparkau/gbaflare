#include "platform.h"

#include <stdexcept>
#include <iostream>

Platform platform;
bool emulator_running = false;

int Platform::init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		printf("could not init SDL\n");
		goto init_failed;
	}

	window = SDL_CreateWindow("GBAFlare", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width*scale_factor, height*scale_factor, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("could not create window\n");
		goto init_failed;
	}

	SDL_SetWindowResizable(window, SDL_FALSE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		printf("could not create renderer\n");
		goto init_failed;
	}

	if (SDL_RenderSetLogicalSize(renderer, width, height) < 0) {
		printf("could not set logical size\n");
		goto init_failed;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!texture) {
		printf("could not create texture\n");
		goto init_failed;
	}

	emulator_running = true;
	return PLATFORM_INIT_SUCCESS;

init_failed:
	if (texture) {
		SDL_DestroyTexture(texture);
	}
	texture = nullptr;

	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}
	renderer = nullptr;

	if (window) {
		SDL_DestroyWindow(window);
	}
	window = nullptr;

	SDL_Quit();

	return PLATFORM_INIT_FAIL;
}


void Platform::render(u16 *pixels)
{
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_Rect r = {0, 0, width, height};
	SDL_RenderFillRect(renderer, &r);

	SDL_UpdateTexture(texture, NULL, pixels, width * sizeof *pixels);

	SDL_RenderCopy(renderer, texture, NULL, NULL);

	SDL_RenderPresent(renderer);
}

void Platform::handle_input()
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			emulator_running = false;
			break;
		}
	}

}
