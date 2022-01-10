#include "ppu.h"
#include "memory.h"
#include "platform.h"

#include <cstring>

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
					if (DISPSTAT() & LCD_VBLANK_IRQ) {
						request_interrupt(IRQ_VBLANK);
					}
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
		if (DISPSTAT() & LCD_HBLANK_IRQ) {
			request_interrupt(IRQ_HBLANK);
		}
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
			if (DISPSTAT() & LCD_VCOUNTER_IRQ) {
				request_interrupt(IRQ_VCOUNTER);
			}
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
		case 3:
			copy_framebuffer_mode3();
			break;
		case 4:
			copy_framebuffer_mode4();
			break;
		case 5:
			copy_framebuffer_mode5();
			break;
	}

	platform.render(framebuffer);

}

void PPU::copy_framebuffer_mode3()
{
	for (std::size_t i = 0; i < FRAMEBUFFER_SIZE; i++) {
		framebuffer[i] = readarr<u16>(vram_data, i*2);
	}
}

void PPU::copy_framebuffer_mode4()
{
	u8 *p = vram_data;

	if (io_read<u8>(IO_DISPCNT) & LCD_FRAME) {
		p += 40_KiB;
	}

	for (std::size_t i = 0; i < FRAMEBUFFER_SIZE; i++) {
		u8 offset = p[i];
		framebuffer[i] = readarr<u16>(palette_data, offset * 2);
	}
}

void PPU::copy_framebuffer_mode5()
{
	u8 *p = vram_data;

	if (io_read<u8>(IO_DISPCNT) & LCD_FRAME) {
		p += 40_KiB;
	}

	const int w = 160;
	const int h = 128;

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			framebuffer[i*LCD_WIDTH + j] = readarr<u16>(p, (i*w + j) * 2);
		}
		for (int j = w; j < LCD_WIDTH; j++) {
			framebuffer[i*LCD_WIDTH + j] = readarr<u16>(palette_data, 0);
		}
	}

	for (int i = h; i < LCD_HEIGHT; i++) {
		for (int j = 0; j < LCD_WIDTH; j++) {
			framebuffer[i*LCD_WIDTH + j] = readarr<u16>(palette_data, 0);
		}
	}
}
