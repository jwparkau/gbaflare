#include "ppu.h"
#include "memory.h"

PPU ppu;

void PPU::copy_framebuffer_mode4()
{
	for (int i = 0; i < 240 * 160; i++) {
		u8 offset = vram_data[i];
		framebuffer[i] = readarr<u16>(palette_data, offset * 2);
	}
}
