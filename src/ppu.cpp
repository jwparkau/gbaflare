#include "ppu.h"
#include "memory.h"
#include "platform.h"
#include "scheduler.h"
#include "dma.h"

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

/* affine bgs are always square */
static const int BG_AFFINE_DIM[4] = {128, 256, 512, 1024};

PPU ppu;
int vblank_flag;

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
				on_hblank();
			}
			break;
		case PPU_IN_HBLANK:
			if (cycles < 960 || cycles >= 1232) {
				if (LY() == 160) {
					ppu_mode = PPU_IN_VBLANK;
					on_vblank();
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
	if (DISPSTAT() & LCD_VBLANK_IRQ) {
		request_interrupt(IRQ_VBLANK);
	}
	DISPSTAT() |= LCD_VBLANK;

	copy_affine_ref();
	dma.on_vblank();
	platform.handle_input();
	io_write<u16>(IO_KEYINPUT, joypad_state);
	platform.render(framebuffer);
	vblank_flag += 1;
}

void PPU::on_hblank()
{
	draw_scanline();
	dma.on_hblank();
}

void PPU::copy_affine_ref()
{
	ref_x[0] = (s32)(io_read<u32>(IO_BG2X_L) << 4) >> 4;
	ref_y[0] = (s32)(io_read<u32>(IO_BG2Y_L) << 4) >> 4;
	ref_x[1] = (s32)(io_read<u32>(IO_BG3X_L) << 4) >> 4;
	ref_y[1] = (s32)(io_read<u32>(IO_BG3Y_L) << 4) >> 4;
}

void PPU::draw_scanline()
{
	u16 backdrop = readarr<u16>(palette_data, 0);

	int ly = LY();

	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		bufferA[i] = {backdrop, MIN_PRIO, 0, false, LAYER_BD};
		obj_buffer[i] = {backdrop, MIN_PRIO, 0, false, LAYER_BD};
	}

	u16 dispcnt = io_read<u16>(IO_DISPCNT);

	int bg_mode = dispcnt & LCD_BGMODE;

	switch (bg_mode) {
		case 0:
			do_bg_mode<0>();
			break;
		case 1:
			do_bg_mode<1>();
			break;
		case 2:
			do_bg_mode<2>();
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

	if (dispcnt & LCD_OBJ) {
		render_sprites();
	}

	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++) {
		framebuffer[i] = bufferA[i].color_555;
		if (!obj_buffer[i].is_transparent && obj_buffer[i].layer != LAYER_BD && obj_buffer[i].priority <= bufferA[i].priority) {
			framebuffer[i] = obj_buffer[i].color_555;
		}
	}

	ref_x[0] += (s32)(s16)io_read<u16>(IO_BG2PB);
	ref_y[0] += (s32)(s16)io_read<u16>(IO_BG2PD);
	ref_x[1] += (s32)(s16)io_read<u16>(IO_BG3PB);
	ref_y[1] += (s32)(s16)io_read<u16>(IO_BG2PD);
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

void PPU::render_affine_bg(int bg, int priority)
{
	int ly = LY();

	u16 bgcnt = io_read<u16>(IO_BG0CNT + bg*2);

	int screen_size = GET_FLAG(bgcnt, BG_SCREEN_SIZE);
	int w = BG_AFFINE_DIM[screen_size];
	int h = BG_AFFINE_DIM[screen_size];

	int sbb = GET_FLAG(bgcnt, BG_SBB);
	int cbb = GET_FLAG(bgcnt, BG_CBB);
	u32 tilemap_base = sbb * 2_KiB;
	u32 tileset_base = cbb * 16_KiB;

	bool affine_wrap = GET_FLAG(bgcnt, BG_OVERFLOW);

	u32 pa = (s32)(s16)io_read<u16>(IO_START + bg*0x10);
	u32 pc = (s32)(s16)io_read<u16>(IO_START + bg*0x10 + 4);

	u32 x2 = ref_x[bg-2];
	u32 y2 = ref_y[bg-2];

	int i = ly;
	int j;
	for (j = 0; j < w; j++) {
		if (j >= LCD_WIDTH) {
			break;
		}

		int bx = (s32)x2 >> 8;
		int by = (s32)y2 >> 8;

		x2 += pa;
		y2 += pc;

		if (affine_wrap) {
			bx = (bx % w + w) % w;
			by = (by % h + h) % h;
		}

		if (bx < 0 || bx >= w || by < 0 || by >= h) {
			continue;
		}

		int tx = bx / 8;
		int ty = by / 8;

		int px = bx % 8;
		int py = by % 8;

		u32 se_offset = tilemap_base + ty*(w/8) + tx;
		int tile_number = readarr<u8>(vram_data, se_offset);

		u32 tile_offset;
		int palette_offset;
		int palette_number;
		tile_offset = tileset_base + (u32)tile_number*64 + py*8 + px;\
		palette_offset = palette_number = readarr<u8>(vram_data, tile_offset);\

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

#define ADD_BACKGROUND(x) \
	if (bg_is_enabled((x))) {\
		priority = GET_FLAG(io_read<u8>(IO_BG0CNT + (x)*2), BG_PRIORITY);\
		bgs_to_render.push_back(std::make_pair(priority, -(x)));\
	}

template<u8 mode> void PPU::do_bg_mode()
{
	int priority;
	std::vector<std::pair<int, int>> bgs_to_render;

	if constexpr (mode == 0) {
		ADD_BACKGROUND(0);
		ADD_BACKGROUND(1);
		ADD_BACKGROUND(2);
		ADD_BACKGROUND(3);
	} else if constexpr (mode == 1) {
		ADD_BACKGROUND(0);
		ADD_BACKGROUND(1);
		ADD_BACKGROUND(2);
	} else if constexpr (mode == 2) {
		ADD_BACKGROUND(2);
		ADD_BACKGROUND(3);
	}

	std::sort(bgs_to_render.begin(), bgs_to_render.end());
	if (bgs_to_render.empty()) {
		return;
	}

	if constexpr (mode == 0) {
		for (auto x : bgs_to_render) {
			render_text_bg(-x.second, x.first);
		}
	} else if constexpr (mode == 1) {
		for (auto x : bgs_to_render) {
			if (-x.second < 2) {
				render_text_bg(-x.second, x.first);
			} else {
				render_affine_bg(-x.second, x.first);
			}
		}
	} else if constexpr (mode == 2) {
		for (auto x : bgs_to_render) {
			render_affine_bg(-x.second, x.first);
		}
	}
}

template void PPU::do_bg_mode<0>();
template void PPU::do_bg_mode<1>();
template void PPU::do_bg_mode<2>();

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

		if (GET_FLAG(attr0, OBJ_DISABLE) && !GET_FLAG(attr0, OBJ_AFFINE)) {
			continue;
		}

		bool is_affine = GET_FLAG(attr0, OBJ_AFFINE);
		bool double_size = GET_FLAG(attr0, OBJ_DOUBLESIZE);

		int objx = GET_FLAG(attr1, OBJ_X);
		int objy = GET_FLAG(attr0, OBJ_Y);

		int shape = GET_FLAG(attr0, OBJ_SHAPE);
		int size = GET_FLAG(attr1, OBJ_SIZE);

		int obj_w = OBJ_REGULAR_WIDTH[shape][size];
		int obj_h = OBJ_REGULAR_HEIGHT[shape][size];

		int box_x, box_y, box_w, box_h;

		box_x = objx;
		box_y = objy;
		box_w = obj_w;
		box_h = obj_h;

		if (is_affine && double_size) {
			box_w *= 2;
			box_h *= 2;
			objx += obj_w / 2;
			objy += obj_h / 2;
		}

		if (box_x >= LCD_WIDTH) {
			box_x -= 512;
			objx -= 512;
		}
		if (box_y >= LCD_HEIGHT) {
			box_y -= 256;
			objy -= 256;
		}

		if (!(box_y <= ly && ly < box_y + box_h)) {
			continue;
		}

		int sprite_y = ly - objy;
		if (!is_affine && GET_FLAG(attr1, OBJ_VFLIP)) {
			sprite_y = obj_h - 1 - sprite_y;
		}

		int sprite_x = box_x - objx;

		bool color_8 = GET_FLAG(attr0, OBJ_PALETTE);
		int tile_start = GET_FLAG(attr2, OBJ_TILENUMBER);
		int palette_bank = GET_FLAG(attr2, OBJ_PALETTE_NUMBER);
		int priority = GET_FLAG(attr2, OBJ_PRIORITY);

		bool hflip = GET_FLAG(attr1, OBJ_HFLIP) && !is_affine;

		u16 x0 = obj_w / 2;
		u16 y0 = obj_h / 2;

		u16 pa, pb, pc, pd;
		u16 x2 = 0, y2 = 0;
		if (is_affine) {
			int affine_index = GET_FLAG(attr1, OBJ_AFFINE_PARAMETER);
			u32 affine_base_addr = 0x20 * affine_index + 6;

			pa = readarr<u16>(oam_data, affine_base_addr);
			pb = readarr<u16>(oam_data, affine_base_addr+8);
			pc = readarr<u16>(oam_data, affine_base_addr+16);
			pd = readarr<u16>(oam_data, affine_base_addr+24);

			x2 = (sprite_x-x0)*pa + (sprite_y-y0)*pb + (x0 << 8);
			y2 = (sprite_x-x0)*pc + (sprite_y-y0)*pd + (y0 << 8);
		}

		int j;
		for (sprite_x = (hflip ? box_w-1 : 0), j = box_x; j < box_x + box_w; sprite_x += (hflip ? -1 : 1), j++) {
			if (j >= LCD_WIDTH) {
				break;
			}

			if (is_affine) {
				sprite_x = (s16)x2 >> 8;
				sprite_y = (s16)y2 >> 8;

				x2 += pa;
				y2 += pc;
			}

			if (j < 0) {
				continue;
			}

			if (sprite_y < 0 || sprite_y >= obj_h) {
				continue;
			}
			if (sprite_x < 0 || sprite_x >= obj_w) {
				continue;
			}

			int tx = sprite_x / 8;
			int px = sprite_x % 8;

			int ty = sprite_y / 8;
			int py = sprite_y % 8;

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
