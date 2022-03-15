#ifndef GBAFLARE_PLATFORM_H
#define GBAFLARE_PLATFORM_H

#include <common/types.h>
#include <semaphore>
#include <atomic>
#include <string>
#include <mutex>

#define SAMPLE_RATE 65536
#define CYCLES_PER_SAMPLE (16 * 1024 * 1024 / SAMPLE_RATE)
#define CYCLES_PER_FRAME 280896
#define SAMPLES_PER_FRAME (CYCLES_PER_FRAME / CYCLES_PER_SAMPLE * 2)
#define AUDIOBUFFER_SIZE SAMPLES_PER_FRAME * 2

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

enum emulator_states {
	EMULATION_STOPPED,
	EMULATION_PAUSED,
	EMULATION_RUNNING,
	EMULATION_NOBIOS
};

constexpr double FPS = 59.72750057;
extern const char *bios_filenames[];
extern const std::string prog_name;

std::string get_data_dir();
std::string get_default_bios_path();
std::string get_default_bios_path(const std::string &s);
int find_bios_file(std::string &s);
void copy_bios_file(std::string &s);
void update_joypad(joypad_buttons button, bool down);

struct EmulatorControl {
	std::atomic_bool emulator_running{};
	std::atomic_bool throttle_enabled = true;
	std::atomic_bool print_fps{};
	std::atomic_bool debug{};

	std::atomic_uint16_t joypad_state = 0xFFFF;

	std::atomic_bool request_pause{};
	std::atomic_bool request_reset{};
	std::atomic_bool request_close{};
	std::atomic_bool request_open{};
	std::atomic_bool request_load_bios{};

	std::atomic_int emulator_state = EMULATION_NOBIOS;

	void on_close();
	void on_pause();
	void toggle_throttle();
	void toggle_print_fps();

	void process_events();
	void process_reset();
	void process_close();
	void process_load();
};

extern EmulatorControl emu_cnt;

struct SharedState {
	u16 framebuffer[FRAMEBUFFER_SIZE];
	s16 audiobuffer[AUDIOBUFFER_SIZE];
	std::string cartridge_filename;
	std::string bios_filename;

	std::mutex lock;
};

extern SharedState shared;

extern u16 framebuffer[FRAMEBUFFER_SIZE];
extern s16 audiobuffer[AUDIOBUFFER_SIZE];

#endif
