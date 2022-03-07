#ifndef GBAFLARE_PLATFORM_H
#define GBAFLARE_PLATFORM_H

#include <common/types.h>
#include <semaphore>
#include <atomic>
#include <mutex>
#include <string>

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

constexpr double FPS = 59.72750057;
extern const char *bios_filenames[];
extern const std::string prog_name;

void platform_on_vblank();
int find_bios_file(std::string &s);
void update_joypad(joypad_buttons button, bool down);

struct EmulatorControl {
	std::atomic_bool emulator_running{};
	std::atomic_bool emulator_paused{};
	std::atomic_bool throttle_enabled = true;
	std::atomic_bool print_fps{};
	std::atomic_int save_state{};
	std::atomic_int load_state{};
	std::atomic_uint16_t joypad_state = 0xFFFF;
	std::atomic_bool do_reset{};
	std::atomic_bool do_close{};
};

extern EmulatorControl emu_cnt;

extern u16 framebuffer[FRAMEBUFFER_SIZE];
extern s16 audiobuffer[AUDIOBUFFER_SIZE];
extern std::atomic_uint32_t audio_buffer_index;



#endif
