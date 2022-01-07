#ifndef GBAFLARE_PPU_H
#define GBAFLARE_PPU_H

#include "types.h"

struct PPU {
	u16 framebuffer[240 * 160];
};

extern PPU ppu;

#endif
