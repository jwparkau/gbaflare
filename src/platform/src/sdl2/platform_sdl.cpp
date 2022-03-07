#include <platform/sdl2/platform_sdl.h>
#include <platform/common/platform.h>
#include <gba/emulator.h>
#include <gba/memory.h>

#include <stdexcept>
#include <iostream>
#include <fstream>

joypad_buttons translate_sym(SDL_Keycode sym);
joypad_buttons translate_button(int button);

Platform platform;

int main(int argc, char **argv)
{
	fprintf(stderr, "GBAFlare - Gameboy Advance Emulator\n");

	const char **fn;
	for (fn = bios_filenames; *fn; fn++) {
		fprintf(stderr, "trying bios file %s\n", *fn);
		std::ifstream f(*fn);
		if (f.good()) {
			break;
		}
	}
	if (*fn) {
		args.bios_filename = std::string(*fn);
	} else {
		int err = find_bios_file(args.bios_filename);
		if (err) {
			fprintf(stderr, "no bios file\n");
			return EXIT_FAILURE;
		}
	}

	load_bios_rom(args.bios_filename);

	int err = platform.init();
	if (err) {
		return EXIT_FAILURE;
	}
	platform.render(framebuffer);

	if (argc >= 2) {
		args.cartridge_filename = std::string(argv[1]);
		emu.init(args);
	}

	emu_cnt.emulator_running = true;

	auto tick_start = std::chrono::steady_clock::now();
	std::chrono::duration<double> frame_duration(1.0 / FPS);

	while (emu_cnt.emulator_running) {
		if (emu_cnt.emulator_paused) {
			platform.wait_for_unpause();
		}
		if (emu.cartridge_loaded) {
			emu.run_one_frame();
			platform_on_vblank();
			platform.handle_input();
			platform.render(framebuffer);
			platform.queue_audio(audiobuffer);
		} else {
			int file = platform.wait_for_cartridge_file(args.cartridge_filename);
			if (file) {
				emu.reset_memory();
				emu.init(args);
			}
		}

		std::chrono::duration<double> sec = std::chrono::steady_clock::now() - tick_start;
		if (emu_cnt.print_fps) {
			fprintf(stderr, "fps: %f\n", 1 / sec.count());
		}

		if (emu_cnt.throttle_enabled) {
			while (std::chrono::steady_clock::now() - tick_start < frame_duration)
				;
		}
		tick_start = std::chrono::steady_clock::now();
	}

	emu_cnt.emulator_paused = false;

	emu.close();

	return 0;
}

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

void Platform::render_black()
{
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void Platform::queue_audio(void *buffer)
{
	if (!audio_device) {
		return;
	}

	SDL_QueueAudio(audio_device, buffer, SAMPLES_PER_FRAME * sizeof(*audiobuffer));
	audio_buffer_index = 0;
}

bool Platform::wait_for_cartridge_file(std::string &s)
{
	fprintf(stderr, "drag and drop in a .gba file\n");
	for (;;) {
		render_black();

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				emu_cnt.emulator_running = false;
				return false;
			}

			if (e.type == SDL_DROPFILE) {
				char *filename = e.drop.file;
				std::string temp = std::string(filename);
				SDL_free(filename);
				if (temp.ends_with(".gba")) {
					s = temp;
					return true;
				} else {
					fprintf(stderr, "cartridge file should end with .gba\n");
				}
			}
		}
		SDL_Delay(16);
	}
}

void Platform::wait_for_unpause()
{
	for (;;) {
		render(framebuffer);

		handle_input();
		if (!emu_cnt.emulator_paused) {
			break;
		}
		SDL_Delay(16);
	}
}


void Platform::handle_input()
{
	SDL_Event e;

	bool toggle_audio = false;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			emu_cnt.emulator_running = false;
			break;
		}

		if (e.type == SDL_KEYUP) {
			switch (e.key.keysym.sym) {
				case SDLK_0:
					emu_cnt.throttle_enabled = !emu_cnt.throttle_enabled;
					toggle_audio = true;
					break;
				case SDLK_p:
					emu_cnt.print_fps = !emu_cnt.print_fps;
					break;
				case SDLK_ESCAPE:
					emu_cnt.emulator_paused = !emu_cnt.emulator_paused;
					toggle_audio = true;
					if (!emu_cnt.emulator_paused) {
						emu_cnt.emulator_paused.notify_one();
					}
					break;
				case SDLK_F1:
					emu_cnt.save_state = 1;
					break;
				case SDLK_F2:
					emu_cnt.load_state = 1;
					break;
				case SDLK_F5:
					emu_cnt.do_reset = true;
					break;
				case SDLK_F12:
					emu_cnt.do_close = true;
					break;
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
		if (emu_cnt.throttle_enabled && !emu_cnt.emulator_paused) {
			SDL_PauseAudioDevice(audio_device, 0);
			SDL_ClearQueuedAudio(audio_device);
			audio_buffer_index = 0;
		} else {
			SDL_PauseAudioDevice(audio_device, 1);
		}
	}
}

joypad_buttons translate_sym(SDL_Keycode sym)
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

joypad_buttons translate_button(int button)
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
