#include "ppu.h"
#include "memory.h"
#include "platform.h"
#include "scheduler.h"

#include <utility>
#include <algorithm>
#include <cstring>
#include <vector>

PPU ppu;

void PPU::step()
{
	cycles += elapsed;

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

	switch (ppu_mode) {
		case PPU_IN_DRAW:
			if (cycles >= 960) {
				ppu_mode = PPU_IN_HBLANK;
				draw_scanline();
			}
			break;
		case PPU_IN_HBLANK:
			if (cycles < 960 || cycles >= 1232) {
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
			if (cycles == 0 || cycles >= 1232) {
				if (LY() == 227) {
					DISPSTAT() &= ~LCD_VBLANK;
				}
				if (LY() == 0) {
					ppu_mode = PPU_IN_DRAW;
				}
			}
			break;
	}

	if (cycles >= 960) {
		if (DISPSTAT() & LCD_HBLANK_IRQ) {
			request_interrupt(IRQ_HBLANK);
		}
		DISPSTAT() |= LCD_HBLANK;
	}

	if (cycles == 0 || cycles >= 1232) {
		DISPSTAT() &= ~LCD_HBLANK;
	}

	if (cycles < 960 || cycles >= 1232) {
		schedule_after(960 - (cycles % 1232));
	} else {
		schedule_after(1232 - cycles);
	}
}

void PPU::draw_scanline()
{
	u16 const backdrop = readarr<u16>(palette_data, 0);

	const int ly = LY();

	for (int i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		framebuffer[i] = backdrop;
	}

	const u8 bg_mode = io_read<u8>(IO_DISPCNT) & LCD_BGMODE;

	switch (bg_mode) {
		case 0:
			do_bg_mode0();
			break;
		case 3:
			copy_framebuffer_mode3();
			break;
		case 4:
			copy_framebuffer_mode4();
			break;
		case 5:
			copy_framebuffer_mode5();
			break;
		default:
			return;
	}
}

void PPU::on_vblank()
{
	platform.handle_input();
	io_write<u16>(IO_KEYINPUT, joypad_state);
	platform.render(framebuffer);
}

bool PPU::bg_is_enabled(int i)
{
	return io_read<u8>(IO_DISPCNT + 1) & BIT(i);
}

void PPU::render_text_bg(int bg)
{
	u16 bgcnt = io_read<u16>(IO_BG0CNT + bg*2);
	u8 cbb = bgcnt >> 2 & BITMASK(2);
	u8 sbb = bgcnt >> 8 & BITMASK(5);
	bool color_8 = bgcnt & BG_PALETTE;
	u8 screen_size = bgcnt >> 14 & BITMASK(2);

	int w, h;
	switch (screen_size) {
		case 0:
			w = 256;
			h = 256;
			break;
		case 1:
			w = 512;
			h = 256;
			break;
		case 2:
			w = 256;
			h = 512;
			break;
		case 3:
			w = 512;
			h = 512;
			break;
	}

	addr_t tilemap_base = sbb * 2_KiB;
	addr_t tileset_base = cbb * 16_KiB;

	int dx = io_read<u16>(IO_BG0HOFS + bg*4);
	int dy = io_read<u16>(IO_BG0VOFS + bg*4);

	int i = LY();
	for (int j = 0; j < LCD_WIDTH; j++) {
		int bx = (dx + j) % w;
		int by = (dy + i) % h;

		int sc = (by / 256)*2 + (bx / 256);
		if (screen_size == 2) {
			sc /= 2;
		}

		int tx = (bx % 256) / 8;
		int ty = (by % 256) / 8;

		int px = bx % 8;
		int py = by % 8;

		u32 se_offset = tilemap_base + sc*2_KiB + 2 * (ty*32 + tx);
		u16 se = readarr<u16>(vram_data, se_offset);

		if (se & SE_HFLIP) {
			px = 7 - px;
		}
		if (se & SE_VFLIP) {
			py = 7 - py;
		}

		u16 tile_number = se & SE_TILENUMBER;

		u32 tile_offset;
		u8 palette_offset;
		u8 palette_number;
		if (color_8) {
			tile_offset = tileset_base + 64 * tile_number + py*8 + px;
			palette_offset = palette_number = readarr<u8>(vram_data, tile_offset);
		} else {
			tile_offset = (tileset_base + 32 * tile_number + py*4 + px/2);
			palette_number = readarr<u8>(vram_data, tile_offset) >> (px%2*4) & BITMASK(4);
			palette_offset = (se >> 12 & BITMASK(4)) * 16 + palette_number;
		}

		// UNSAFE -- at this point bg mode must be 0, 1, 2
		if (palette_number != 0 && tile_offset < 0x10000) {
			framebuffer[i*LCD_WIDTH + j] = readarr<u16>(palette_data, palette_offset * 2);
		}
	}
}

void PPU::do_bg_mode0()
{
	// TODO: this wont work with sprites -- fix
	std::vector<std::pair<int, int>> bgs_to_render;
	for (int i = 0; i < 4; i++) {
		if (bg_is_enabled(i)) {
			int prio = io_read<u8>(IO_BG0CNT + i*2) & BG_PRIORITY;
			bgs_to_render.push_back(std::make_pair(prio, -i));
		}
	}

	std::sort(bgs_to_render.begin(), bgs_to_render.end());
	if (bgs_to_render.empty()) {
		return;
	}

	for (auto x : bgs_to_render) {
		render_text_bg(-x.second);
	}
}

void PPU::copy_framebuffer_mode3()
{
	const int ly = LY();

	for (int i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		framebuffer[i] = readarr<u16>(vram_data, i*2);
	}
}

void PPU::copy_framebuffer_mode4()
{
	u8 *p = vram_data;

	if (io_read<u8>(IO_DISPCNT) & LCD_FRAME) {
		p += 40_KiB;
	}

	const int ly = LY();

	for (int i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
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

	const int ly = LY();

	if (ly < h) {
		for (int j = 0; j < w; j++) {
			framebuffer[ly*LCD_WIDTH + j] = readarr<u16>(p, (ly*w + j) * 2);
		}
		for (int j = w; j < LCD_WIDTH; j++) {
			framebuffer[ly*LCD_WIDTH + j] = readarr<u16>(palette_data, 0);
		}
	} else {
		for (int j = 0; j < LCD_WIDTH; j++) {
			framebuffer[ly*LCD_WIDTH + j] = readarr<u16>(palette_data, 0);
		}
	}

	for (int i = h; i < LCD_HEIGHT; i++) {
	}
}
