#ifndef GBAFLARE_PLATFORM_H
#define GBAFLARE_PLATFORM_H

#include "types.h"

#define LCD_WIDTH 240l
#define LCD_HEIGHT 160l
#define FRAMEBUFFER_SIZE 38400l

int platform_init();
void platform_on_vblank();

extern bool emulator_running;
extern bool throttle_enabled;
extern bool print_fps;

extern u16 joypad_state;

extern u16 framebuffer[FRAMEBUFFER_SIZE];


#ifdef PLATFORM_USE_SDL2
#include "platform_sdl.h"
#endif

extern Platform platform;

#endif
