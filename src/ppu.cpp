#include "ppu.h"
#include "memory.h"
#include "platform.h"

PPU ppu;

void PPU::step()
{
	switch (ppu_mode) {
		case PPU_IN_DRAW:
			if (cycles == 960) {
				ppu_mode = PPU_IN_HBLANK;
			}
			break;
		case PPU_IN_HBLANK:
			if (cycles == 0) {
				if (LY() == 160) {
					ppu_mode = PPU_IN_VBLANK;
					on_vblank();
					DISPSTAT() |= LCD_VBLANK;
				} else {
					ppu_mode = PPU_IN_DRAW;
				}
			}
			break;
		case PPU_IN_VBLANK:
			if (cycles == 0) {
				if (LY() == 227) {
					DISPSTAT() &= ~LCD_VBLANK;
				}
				if (LY() == 0) {
					ppu_mode = PPU_IN_DRAW;
				}
			}
			break;
	}

	if (cycles == 960) {
		DISPSTAT() |= LCD_HBLANK;
	}

	if (cycles == 0) {
		DISPSTAT() &= ~LCD_HBLANK;
	}

	cycles += 1;
	if (cycles >= 1232) {
		cycles = 0;

		LY() += 1;
		if (LY() >= 228) {
			LY() = 0;
		}

		if (LY() == LYC()) {
			DISPSTAT() |= LCD_VCOUNTER;
		} else {
			DISPSTAT() &= ~LCD_VCOUNTER;
		}
	}
}

void PPU::on_vblank()
{
	const u8 bg_mode = io_read<u8>(IO_DISPCNT) & LCD_BGMODE;

	switch (bg_mode) {
		case 4:
		default:
			copy_framebuffer_mode4();
			platform.render(framebuffer);
			break;
	}

}

void PPU::copy_framebuffer_mode4()
{
	for (int i = 0; i < 240 * 160; i++) {
		u8 offset = vram_data[i];
		framebuffer[i] = readarr<u16>(palette_data, offset * 2);
	}
}
