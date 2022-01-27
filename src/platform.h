#ifndef GBAFLARE_PLATFORM_H
#define GBAFLARE_PLATFORM_H

#include "types.h"
#include <semaphore>
#include <atomic>
#include <mutex>

#define LCD_WIDTH 240l
#define LCD_HEIGHT 160l
#define FRAMEBUFFER_SIZE 38400l

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


#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.h"
#endif

extern Platform platform;

#endif
