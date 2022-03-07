#ifndef GBAFLARE_PLATFORM_SDL2_H
#define GBAFLARE_PLATFORM_SDL2_H

#include <SDL.h>

#include "types.h"
#include <string>

enum platform_error {
	PLATFORM_INIT_SUCCESS,
	PLATFORM_INIT_FAIL
};

struct Platform {
	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};
	SDL_GameController *controller{};
	SDL_AudioDeviceID audio_device{};
	SDL_AudioSpec audio_spec_want{};
	SDL_AudioSpec audio_spec_have{};

	void add_controller();
	void remove_controller();

	const int width = 240;
	const int height = 160;
	const int scale_factor = 4;

	int init();

	void render(u16 *pixels);
	void render_black();
	void handle_input();
	void queue_audio(void *buffer);
	bool wait_for_cartridge_file(std::string &s);
	void wait_for_unpause();
};

extern Platform platform;

#endif
