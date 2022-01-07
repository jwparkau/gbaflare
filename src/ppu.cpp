#include "ppu.h"
#include "memory.h"

PPU ppu;

void PPU::copy_framebuffer_mode4()
{
	for (int i = 0; i < 240 * 160; i++) {
		u32 offset = Memory::read8(VRAM_START + i);
		framebuffer[i] = Memory::read16(offset * 2 + 0x05000000);
	}
}
