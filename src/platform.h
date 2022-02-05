#ifndef GBAFLARE_PLATFORM_H
#define GBAFLARE_PLATFORM_H

#include "types.h"
#include <semaphore>
#include <atomic>
#include <mutex>

#define LCD_WIDTH 240l
#define LCD_HEIGHT 160l
#define FRAMEBUFFER_SIZE 38400l

#define SAMPLE_RATE 32768
#define CYCLES_PER_SAMPLE (16 * 1024 * 1024 / SAMPLE_RATE)
#define CYCLES_PER_FRAME 280896
#define SAMPLES_PER_FRAME (CYCLES_PER_FRAME / CYCLES_PER_SAMPLE * 2)
#define AUDIOBUFFER_SIZE SAMPLES_PER_FRAME * 2

constexpr double FPS = 59.72750057;

int platform_init();
void platform_on_vblank();

extern std::atomic_bool emulator_running;
extern std::atomic_bool throttle_enabled;
extern std::atomic_bool print_fps;

extern std::atomic_uint16_t joypad_state;

extern u16 framebuffer[FRAMEBUFFER_SIZE];
extern u16 real_framebuffer[FRAMEBUFFER_SIZE];

extern std::binary_semaphore frame_rendered, frame_drawn;
extern std::mutex f_lock;

extern s16 audiobuffer[AUDIOBUFFER_SIZE];
extern s16 real_audiobuffer[AUDIOBUFFER_SIZE];
extern std::atomic_uint32_t audio_buffer_index;

#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.h"
#endif

extern Platform platform;

#endif
