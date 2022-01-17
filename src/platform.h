#ifndef GBAFLARE_PLATFORM_H
#define GBAFLARE_PLATFORM_H

#include <SDL2/SDL.h>

#include "types.h"

enum platform_error {
	PLATFORM_INIT_SUCCESS,
	PLATFORM_INIT_FAIL
};

struct Platform {
	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};

	const int width = 240;
	const int height = 160;
	const int scale_factor = 8;

	int init();

	void render(u16 *pixels);
	void handle_input();
};

extern Platform platform;

extern bool emulator_running;
extern bool throttle_enabled;

extern u16 joypad_state;

#endif
