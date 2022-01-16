#include "ppu.h"
#include "memory.h"
#include "platform.h"
#include "scheduler.h"

#include <utility>
#include <algorithm>
#include <cstring>
#include <vector>

static const u8 OBJ_REGULAR_WIDTH[3][4] = {
	{8, 16, 32, 64},
	{16, 32, 32, 64},
	{8, 8, 16, 32}
};

static const u8 OBJ_REGULAR_HEIGHT[3][4] = {
	{8, 16, 32, 64},
	{8, 8, 16, 32},
	{16, 32, 32, 64}
};

static const int BG_REGULAR_WIDTH[4] = {256, 512, 256, 512};
static const int BG_REGULAR_HEIGHT[4] = {256, 256, 512, 512};

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

void PPU::on_vblank()
{
	platform.handle_input();
	io_write<u16>(IO_KEYINPUT, joypad_state);
	platform.render(framebuffer);
}


void PPU::draw_scanline()
{
	u16 backdrop = readarr<u16>(palette_data, 0);

	int ly = LY();

	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		bufferA[i] = {backdrop, MIN_PRIO, 0, false, LAYER_BD};
		obj_buffer[i] = {backdrop, MIN_PRIO, 0, false, LAYER_BD};
	}

	int bg_mode = DISPCNT() & LCD_BGMODE;

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

	render_sprites();

	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		framebuffer[i] = bufferA[i].color_555;
		if (!obj_buffer[i].is_transparent && obj_buffer[i].layer != LAYER_BD && obj_buffer[i].priority <= bufferA[i].priority) {
			framebuffer[i] = obj_buffer[i].color_555;
		}
	}
}

void PPU::render_text_bg(int bg, int priority)
{
	u16 bgcnt = io_read<u16>(IO_BG0CNT + bg*2);

	int screen_size = GET_FLAG(bgcnt, BG_SCREEN_SIZE);
	int w = BG_REGULAR_WIDTH[screen_size];
	int h = BG_REGULAR_HEIGHT[screen_size];

	int sbb = GET_FLAG(bgcnt, BG_SBB);
	int cbb = GET_FLAG(bgcnt, BG_CBB);
	u32 tilemap_base = sbb * 2_KiB;
	u32 tileset_base = cbb * 16_KiB;

	u16 dx = io_read<u16>(IO_BG0HOFS + bg*4);
	u16 dy = io_read<u16>(IO_BG0VOFS + bg*4);

	bool color_8 = GET_FLAG(bgcnt, BG_PALETTE);

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

		u32 se_offset = tilemap_base + sc*2_KiB + 2*(ty*32 + tx);
		u16 se = readarr<u16>(vram_data, se_offset);

		if (GET_FLAG(se, SE_HFLIP)) {
			px = 7 - px;
		}
		if (GET_FLAG(se, SE_VFLIP)) {
			py = 7 - py;
		}

		int tile_number = GET_FLAG(se, SE_TILENUMBER);
		int palette_bank = GET_FLAG(se, SE_PALETTE_NUMBER);

#define GET_PALETTE_OFFSET \
		u32 tile_offset;\
		int palette_offset;\
		int palette_number;\
		\
		if (color_8) {\
			tile_offset = tileset_base + (u32)tile_number*64 + py*8 + px;\
			palette_offset = palette_number = readarr<u8>(vram_data, tile_offset);\
		} else {\
			tile_offset = (tileset_base + (u32)tile_number*32 + py*4 + px/2);\
			palette_number = readarr<u8>(vram_data, tile_offset) >> (px%2*4) & BITMASK(4);\
			palette_offset = palette_bank * 16 + palette_number;\
		}

		GET_PALETTE_OFFSET;

		u16 color_555 = readarr<u16>(palette_data, palette_offset *2);

		if (palette_number != 0 && tile_offset < 0x10000) {
			pixel_info pixel{color_555, priority, bg, palette_number == 0, bg};
			pixel_info other = bufferA[i*LCD_WIDTH + j];

			if (pixel.priority < other.priority || (pixel.priority == other.priority && pixel.bg < other.bg)) {
				bufferA[i*LCD_WIDTH + j] = pixel;
			}
		}
	}
}

bool PPU::bg_is_enabled(int i)
{
	return io_read<u8>(IO_DISPCNT + 1) & BIT(i);
}

void PPU::do_bg_mode0()
{
	int priority;

	std::vector<std::pair<int, int>> bgs_to_render;
	for (int i = 0; i < 4; i++) {
		if (bg_is_enabled(i)) {
			priority = GET_FLAG(io_read<u8>(IO_BG0CNT + i*2), BG_PRIORITY);
			bgs_to_render.push_back(std::make_pair(priority, -i));
		}
	}

	std::sort(bgs_to_render.begin(), bgs_to_render.end());
	if (bgs_to_render.empty()) {
		return;
	}

	for (auto x : bgs_to_render) {
		render_text_bg(-x.second, x.first);
	}
}

#define BITMAP_BG_START \
	int ly = LY();\
	int bg = 2;\
	u16 bgcnt = io_read<u16>(IO_BG0CNT + bg*2);\
	int priority = GET_FLAG(bgcnt, BG_PRIORITY);

#define BITMAP_GET_FRAME \
	u8 *p = vram_data;\
	if (DISPCNT() & LCD_FRAME) {\
		p += 40_KiB;\
	}

void PPU::copy_framebuffer_mode3()
{
	BITMAP_BG_START;

	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		u16 color_555 = readarr<u16>(vram_data, i*2);
		bufferA[i] = {color_555, priority, bg, false, LAYER_BG2};
	}
}

void PPU::copy_framebuffer_mode4()
{
	BITMAP_BG_START;
	BITMAP_GET_FRAME;

	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		u16 color_555 = readarr<u16>(palette_data, p[i]*2);
		bufferA[i] = {color_555, priority, bg, false, LAYER_BG2};
	}
}

void PPU::copy_framebuffer_mode5()
{
	BITMAP_BG_START;
	BITMAP_GET_FRAME;

	const int w = 160;
	const int h = 128;

	if (ly < h) {
		for (int j = 0; j < w; j++) {
			u16 color_555 = readarr<u16>(p, (ly*w + j) * 2);
			bufferA[ly*LCD_WIDTH + j] = {color_555, priority, bg, false, LAYER_BG2};
		}
	}
}

void PPU::render_sprites()
{
	int ly = LY();

	for (int i = MAX_SPRITES - 1; i >= 0; i--) {
		u16 attr0 = readarr<u16>(oam_data, i*8);
		u16 attr1 = readarr<u16>(oam_data, i*8 + 2);
		u16 attr2 = readarr<u16>(oam_data, i*8 + 4);

		if (GET_FLAG(attr0, OBJ_AFFINE)) {
			continue;
		}

		if (GET_FLAG(attr0, OBJ_DISABLE) && !GET_FLAG(attr0, OBJ_AFFINE)) {
			continue;
		}


		int objx = GET_FLAG(attr1, OBJ_X);
		int objy = GET_FLAG(attr0, OBJ_Y);

		if (objx >= LCD_WIDTH) {
			objx -= 512;
		}

		if (objy >= LCD_HEIGHT) {
			objy -= 256;
		}

		int shape = GET_FLAG(attr0, OBJ_SHAPE);
		int size = GET_FLAG(attr1, OBJ_SIZE);

		int obj_w = OBJ_REGULAR_WIDTH[shape][size];
		int obj_h = OBJ_REGULAR_HEIGHT[shape][size];

		if (!(objy <= ly && ly < objy + obj_h)) {
			continue;
		}

		int sprite_y = ly - objy;
		if (GET_FLAG(attr1, OBJ_VFLIP)) {
			sprite_y = obj_h - 1 - sprite_y;
		}

		int ty = sprite_y / 8;
		int py = sprite_y % 8;

		bool color_8 = GET_FLAG(attr0, OBJ_PALETTE);
		int tile_start = GET_FLAG(attr2, OBJ_TILENUMBER);
		int palette_bank = GET_FLAG(attr2, OBJ_PALETTE_NUMBER);
		int priority = GET_FLAG(attr2, OBJ_PRIORITY);

		bool hflip = GET_FLAG(attr1, OBJ_HFLIP);

		for (int sprite_x = (hflip ? obj_w-1 : 0), j = objx; sprite_x != (hflip ? -1 : obj_w); sprite_x += (hflip ? -1 : 1), j++) {
			if (j < 0) {
				continue;
			}

			if (j >= LCD_WIDTH) {
				break;
			}

			int tx = sprite_x / 8;
			int px = sprite_x % 8;

			int tile_number;

			if (DISPCNT() & LCD_OBJ_DIM) {
				if (color_8) {
					tile_number = tile_start + ty*2*(obj_w/8) + 2*tx;
				} else {
					tile_number = tile_start + ty*(obj_w/8) + tx;
				}
			} else {
				if (color_8) {
					tile_number = tile_start + ty*32 + 2*tx;
				} else {
					tile_number = tile_start + ty*32 + tx;
				}
			}

			tile_number %= 1024;
			u32 tileset_base = 0x10000;

			GET_PALETTE_OFFSET;

			u16 color_555 = readarr<u16>(palette_data, 0x200 + palette_offset *2);

			if (palette_number != 0) {
				pixel_info pixel{color_555, priority, i, false, LAYER_OBJ};
				pixel_info other = obj_buffer[ly*LCD_WIDTH + j];

				if (pixel.priority < other.priority || (pixel.priority == other.priority && pixel.bg < other.bg)) {
					obj_buffer[ly*LCD_WIDTH + j] = pixel;
				}
			}
		}
	}
}
