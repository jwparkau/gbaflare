#include "platform.h"

#include <stdexcept>
#include <iostream>

Platform platform;
bool emulator_running = false;
u16 joypad_state = 0xFFFF;
bool throttle_enabled = false;

enum joypad_buttons {
	BUTTON_A,
	BUTTON_B,
	BUTTON_SELECT,
	BUTTON_START,
	BUTTON_RIGHT,
	BUTTON_LEFT,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_R,
	BUTTON_L,
	NOT_MAPPED
};

static joypad_buttons translate_sym(SDL_Keycode sym);
static void update_joypad(joypad_buttons button, bool down);

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

		if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_0) {
			throttle_enabled ^= 1;
		}

		switch (e.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				update_joypad(translate_sym(e.key.keysym.sym), e.type == SDL_KEYDOWN);
				break;
		}
	}

}

static void update_joypad(joypad_buttons button, bool down)
{
	if (button != NOT_MAPPED) {
		if (down) {
			joypad_state &= ~BIT(button);
		} else {
			joypad_state |= BIT(button);
		}
	}
}

static joypad_buttons translate_sym(SDL_Keycode sym)
{
	switch (sym) {
		case SDLK_RIGHT:
			return BUTTON_RIGHT;
		case SDLK_LEFT:
			return BUTTON_LEFT;
		case SDLK_UP:
			return BUTTON_UP;
		case SDLK_DOWN:
			return BUTTON_DOWN;
		case SDLK_x:
			return BUTTON_A;
		case SDLK_z:
			return BUTTON_B;
		case SDLK_1:
		case SDLK_RETURN:
			return BUTTON_START;
		case SDLK_2:
		case SDLK_BACKSPACE:
			return BUTTON_SELECT;
		case SDLK_s:
			return BUTTON_R;
		case SDLK_a:
			return BUTTON_L;
		default:
			return NOT_MAPPED;
	}
}
