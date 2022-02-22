#include "platform.h"

#include <stdexcept>
#include <iostream>

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
static joypad_buttons translate_button(int button);
static void update_joypad(joypad_buttons button, bool down);

int Platform::init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
		fprintf(stderr, "could not init SDL\n");
		goto init_failed;
	}

	window = SDL_CreateWindow("GBAFlare", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width*scale_factor, height*scale_factor, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		fprintf(stderr, "could not create window\n");
		goto init_failed;
	}

	SDL_SetWindowResizable(window, SDL_TRUE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		fprintf(stderr, "could not create renderer\n");
		goto init_failed;
	}

	if (SDL_RenderSetLogicalSize(renderer, width, height) < 0) {
		fprintf(stderr, "could not set logical size\n");
		goto init_failed;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!texture) {
		fprintf(stderr, "could not create texture\n");
		goto init_failed;
	}

	add_controller();

	audio_spec_want.freq = SAMPLE_RATE;
	audio_spec_want.format = AUDIO_S16SYS;
	audio_spec_want.channels = 2;
	audio_spec_want.samples = SAMPLES_PER_FRAME + 100;
	audio_spec_want.callback = nullptr;

	audio_device = SDL_OpenAudioDevice(nullptr, false, &audio_spec_want, &audio_spec_have, 0);
	if (!audio_device) {
		fprintf(stderr, "could not init audio device\n");
		goto init_failed;
	}

	fprintf(stderr, "audio: got sample rate %d\n", audio_spec_have.freq);
	fprintf(stderr, "audio: got buffer size %d\n", audio_spec_have.samples);

	SDL_PauseAudioDevice(audio_device, 0);

	render(real_framebuffer);
	emulator_running = true;
	return PLATFORM_INIT_SUCCESS;

init_failed:
	if (audio_device) {
		SDL_CloseAudioDevice(audio_device);
	}
	audio_device = 0;

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

void Platform::add_controller()
{
	if (controller) {
		return;
	}

	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			controller = SDL_GameControllerOpen(i);
			break;
		}
	}
}

void Platform::remove_controller()
{
	if (controller) {
		SDL_GameControllerClose(controller);
		controller = nullptr;
	}
}

void Platform::render(u16 *pixels)
{
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
	SDL_RenderClear(renderer);

	int pitch;
	u16 *p = NULL;
	SDL_LockTexture(texture, NULL, (void **)&p, &pitch);
	std::memcpy(p, pixels, width * height * sizeof *p);
	SDL_UnlockTexture(texture);

	SDL_RenderCopy(renderer, texture, NULL, NULL);

	SDL_RenderPresent(renderer);
}

void Platform::queue_audio()
{
	if (!audio_device) {
		return;
	}

	SDL_QueueAudio(audio_device, real_audiobuffer, SAMPLES_PER_FRAME * sizeof(*real_audiobuffer));
	audio_buffer_index = 0;
}

void Platform::wait_for_cartridge_file(std::string &s)
{
	fprintf(stderr, "drag and drop in a .gba file\n");
	while (s.length() == 0) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				return;
			}

			if (e.type == SDL_DROPFILE) {
				char *filename = e.drop.file;
				std::string temp = std::string(filename);
				SDL_free(filename);
				if (temp.ends_with(".gba")) {
					s = temp;
					return;
				} else {
					fprintf(stderr, "cartridge file should end with .gba\n");
				}
			}
		}

		SDL_Delay(16);
	}

	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}


void Platform::handle_input()
{
	SDL_Event e;

	bool toggle_audio = false;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			emulator_running = false;
			break;
		}

		if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_0) {
			throttle_enabled = !throttle_enabled;
			toggle_audio = true;
		} else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_p) {
			print_fps = !print_fps;
		} else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_ESCAPE) {
			emulator_paused = !emulator_paused;
			toggle_audio = true;
			if (!emulator_paused) {
				emulator_paused.notify_one();
			}
		}

		switch (e.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				update_joypad(translate_sym(e.key.keysym.sym), e.type == SDL_KEYDOWN);
				break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				update_joypad(translate_button(e.cbutton.button), e.type == SDL_CONTROLLERBUTTONDOWN);
				break;
			case SDL_CONTROLLERDEVICEADDED:
				add_controller();
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				remove_controller();
				break;

		}
	}

	if (toggle_audio) {
		if (throttle_enabled && !emulator_paused) {
			SDL_PauseAudioDevice(audio_device, 0);
			SDL_ClearQueuedAudio(audio_device);
			audio_buffer_index = 0;
		} else {
			SDL_PauseAudioDevice(audio_device, 1);
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

static joypad_buttons translate_button(int button)
{
	switch (button) {
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			return BUTTON_RIGHT;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			return BUTTON_LEFT;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			return BUTTON_UP;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			return BUTTON_DOWN;
		case SDL_CONTROLLER_BUTTON_A:
			return BUTTON_A;
		case SDL_CONTROLLER_BUTTON_B:
		case SDL_CONTROLLER_BUTTON_X:
			return BUTTON_B;
		case SDL_CONTROLLER_BUTTON_START:
			return BUTTON_START;
		case SDL_CONTROLLER_BUTTON_BACK:
			return BUTTON_SELECT;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			return BUTTON_R;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			return BUTTON_L;
		default:
			return NOT_MAPPED;
	}
}
