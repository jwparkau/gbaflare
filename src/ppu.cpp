#include "ppu.h"
#include "memory.h"
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

static const int BG_AFFINE_WIDTH[4] = {128, 256, 512, 1024};
static const int BG_AFFINE_HEIGHT[4] = {128, 256, 512, 1024};

PPU ppu;
int vblank_flag;

#define SET_AND_REQ_IRQ(x) \
	if (DISPSTAT() & LCD_##x##_IRQ) {\
		request_interrupt(IRQ_##x);\
	}\
	DISPSTAT() |= LCD_##x;

void PPU::step()
{
	cycles += elapsed;

	if (cycles >= 1232) {
		cycles = 0;

		LY() += 1;
		if (LY() >= 228) {
			LY() = 0;
		}

		ly = LY();

		if (ly == LYC()) {
			SET_AND_REQ_IRQ(VCOUNTER);
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
				if (ly == 160) {
					ppu_mode = PPU_IN_VBLANK;
					on_vblank();
				} else {
					ppu_mode = PPU_IN_DRAW;
				}
			}
			break;
		case PPU_IN_VBLANK:
			if (cycles == 0 || cycles >= 1232) {
				if (ly == 227) {
					DISPSTAT() &= ~LCD_VBLANK;
				}
				if (ly == 0) {
					ppu_mode = PPU_IN_DRAW;
				}
			}
			break;
	}

	if (cycles >= 960) {
		SET_AND_REQ_IRQ(HBLANK);
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
	SET_AND_REQ_IRQ(VBLANK);
	copy_affine_ref();

	dma.on_vblank();

	platform_on_vblank();

	vblank_flag += 1;
}

void PPU::copy_affine_ref()
{
	ref_x[0] = (s32)(io_read<u32>(IO_BG2X_L) << 4) >> 4;
	ref_y[0] = (s32)(io_read<u32>(IO_BG2Y_L) << 4) >> 4;
	ref_x[1] = (s32)(io_read<u32>(IO_BG3X_L) << 4) >> 4;
	ref_y[1] = (s32)(io_read<u32>(IO_BG3Y_L) << 4) >> 4;
}

void PPU::on_hblank()
{
	draw_scanline();
	dma.on_hblank();
}

#define DO_BLEND_GENERIC(x, e) \
	aX = GET_FLAG(a.color, COLOR_##x);\
	nX = at_most((e), 31);\
	SET_FLAG(color, COLOR_##x, nX);

#define DO_BLEND_ALPHA(x) DO_BLEND_GENERIC(x, eva*aX/16 + (GET_FLAG(b.color, COLOR_##x))*evb/16)
#define DO_BLEND_INC(x) DO_BLEND_GENERIC(x, aX + (31-aX)*evy/16)
#define DO_BLEND_DEC(x) DO_BLEND_GENERIC(x, aX - aX*evy/16)

#define DO_BLEND(f) \
	if (blend_mode == BLEND_##f) {\
		int nX, aX;\
		DO_BLEND_##f(R);\
		DO_BLEND_##f(G);\
		DO_BLEND_##f(B);\
	}


void PPU::draw_scanline()
{
	dispcnt = io_read<u16>(IO_DISPCNT);

	u16 backdrop = readarr<u16>(palette_data, 0);
	pixel_info backdrop_info =  {backdrop, 4, 0, false, LAYER_BD, false, false};
	for ITERATE_SCANLINE {
		bufferA[i] = bufferB[i] = obj_buffer[i] = backdrop_info;
	}

	setup_windows();

	if (dispcnt & LCD_OBJ) {
		render_sprites();
	}

	switch (GET_FLAG(dispcnt, LCD_BGMODE)) {
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

	for ITERATE_SCANLINE {
		auto pixel = obj_buffer[i];

		if (pixel.is_transparent)
			continue;

		if (pixel.layer == LAYER_BD)
			continue;

		int blend;
		if (!should_push_pixel(LAYER_OBJ, j, blend))
			continue;

		pixel.enable_blending = blend;

		if (pixel.priority <= bufferA[i].priority) {
			bufferB[i] = bufferA[i];
			bufferA[i] = pixel;
		} else if (pixel.priority <= bufferB[i].priority) {
			bufferB[i] = pixel;
		}
	}

	u16 blendcnt = io_read<u16>(IO_BLDCNT);

	u16 blendalpha = io_read<u16>(IO_BLDALPHA);
	int eva = at_most(GET_FLAG(blendalpha, BLEND_EVA), 16);
	int evb = at_most(GET_FLAG(blendalpha, BLEND_EVB), 16);

	u8 bldy = io_read<u8>(IO_BLDY);
	int evy = at_most(GET_FLAG(bldy, BLEND_EVY), 16);

	u16 color;
	for (u32 i = ly * LCD_WIDTH, j = 0; j < LCD_WIDTH; j++, framebuffer[i++] = color) {
		auto &a = bufferA[i];
		auto &b = bufferB[i];

		color = a.color;

		int blend_mode = GET_FLAG(blendcnt, BLEND_MODE);

		if (!a.enable_blending)
			continue;
		if (a.is_transparent)
			continue;

		if (a.layer == LAYER_OBJ && a.force_alpha && !b.is_transparent && (blendcnt & BIT(b.layer + 8))) {
			blend_mode = BLEND_ALPHA;
		} else {
			if (!(blendcnt & BIT(a.layer)))
				continue;
		}

		if (blend_mode == BLEND_NONE)
			continue;

		if (blend_mode == BLEND_ALPHA && (b.is_transparent || !(blendcnt & BIT(b.layer + 8))))
			continue;

		DO_BLEND(ALPHA);
		DO_BLEND(INC);
		DO_BLEND(DEC);
	}

	ref_x[0] += (s32)(s16)io_read<u16>(IO_BG2PB);
	ref_y[0] += (s32)(s16)io_read<u16>(IO_BG2PD);
	ref_x[1] += (s32)(s16)io_read<u16>(IO_BG3PB);
	ref_y[1] += (s32)(s16)io_read<u16>(IO_BG2PD);
}

void PPU::setup_windows()
{
	setup_window(0);
	setup_window(1);
	for ITERATE_LINE {
		obj_window[j] = false;
	}
	objwindow_enabled = dispcnt & LCD_OBJWINDOW;
	winout_enabled = objwindow_enabled || windows[0].enabled || windows[1].enabled;
}

void PPU::setup_window(int n)
{
	windows[n].enabled = false;

	if (dispcnt & (LCD_WINDOW0 * BIT(n))) {
		bool enabled = true;

		u16 winh = io_read<u16>(IO_WIN0H + 2*(n));
		u16 winv = io_read<u16>(IO_WIN0V + 2*(n));

		int l = GET_FLAG(winh, WINDOW_LEFT);
		int r = GET_FLAG(winh, WINDOW_RIGHT);
		int t = GET_FLAG(winv, WINDOW_TOP);
		int b = GET_FLAG(winv, WINDOW_BOTTOM);

		bool y_in_window;

		if (t <= b) {
			y_in_window = (t <= ly && ly < b);
		} else {
			y_in_window = (t <= ly || ly < b);
		}

		windows[n] = {l, r, t, b, enabled, y_in_window};
	}
}


#define GET_BG_PROPERTIES(x) \
	u16 bgcnt = io_read<u16>(IO_BG0CNT + bg*2);\
	int screen_size = GET_FLAG(bgcnt, BG_SCREEN_SIZE);\
	int w = x##_WIDTH[screen_size];\
	int h = x##_HEIGHT[screen_size];\
	int sbb = GET_FLAG(bgcnt, BG_SBB);\
	int cbb = GET_FLAG(bgcnt, BG_CBB);\
	u32 tilemap_base = sbb * 2_KiB;\
	u32 tileset_base = cbb * 16_KiB;

#define GET_PALETTE_OFFSET(x) \
	u32 tile_offset;\
	int palette_offset;\
	int palette_number;\
	if (color_8) {\
		tile_offset = tileset_base + (u32)tile_number*(x) + py*8 + px;\
		palette_offset = palette_number = readarr<u8>(vram_data, tile_offset);\
	} else {\
		tile_offset = (tileset_base + (u32)tile_number*32 + py*4 + px/2);\
		palette_number = readarr<u8>(vram_data, tile_offset) >> (px%2*4) & BITMASK(4);\
		palette_offset = palette_bank * 16 + palette_number;\
	}

#define GET_PALETTE_OFFSET_SPRITE GET_PALETTE_OFFSET(32)
#define GET_PALETTE_OFFSET_BG_REGULAR GET_PALETTE_OFFSET(64)
#define GET_PALETTE_OFFSET_BG_AFFINE GET_PALETTE_OFFSET(64);

#define PUSH_BG_PIXEL \
	u16 color = readarr<u16>(palette_data, palette_offset*2);\
	if (palette_number != 0 && tile_offset < 0x10000) {\
		int blend;\
		if (should_push_pixel(bg, j, blend)) {\
			pixel_info pixel{color, priority, bg, false, bg, blend, false};\
			pixel_info other = bufferA[i*LCD_WIDTH + j];\
			if (pixel.priority < other.priority || (pixel.priority == other.priority && pixel.bg < other.bg)) {\
				bufferA[i*LCD_WIDTH + j] = pixel;\
				bufferB[i*LCD_WIDTH + j] = other;\
			}\
		}\
	}

#define CHECK_WINDOW(k) \
	if (windows[k].enabled && windows[k].y_in_window) {\
		bool x_in_window;\
		if (windows[k].l <= windows[k].r) {\
			x_in_window = (windows[k].l <= x && x < windows[k].r);\
		} else {\
			x_in_window = (windows[k].l <= x || x < windows[k].r);\
		}\
		if (x_in_window) {\
			u8 wincnt = io_data[IO_WININ - IO_START + (k)];\
			blend = wincnt & BIT(5);\
			return wincnt & BIT(bg);\
		}\
	}


bool PPU::should_push_pixel(int bg, int x, int &blend)
{
	if (!winout_enabled) {
		blend = 1;
		return true;
	}

	CHECK_WINDOW(0);
	CHECK_WINDOW(1);

	if (objwindow_enabled && obj_window[x]) {
		blend = io_data[IO_WINOUT -  IO_START + 1] & BIT(5);
		return io_data[IO_WINOUT -  IO_START + 1] & BIT(bg);
	}

	blend = io_data[IO_WINOUT - IO_START] & BIT(5);
	return io_data[IO_WINOUT - IO_START] & BIT(bg);
}

void PPU::render_text_bg(int bg, int priority)
{
	GET_BG_PROPERTIES(BG_REGULAR);

	u16 dx = io_read<u16>(IO_BG0HOFS + bg*4);
	u16 dy = io_read<u16>(IO_BG0VOFS + bg*4);

	bool color_8 = GET_FLAG(bgcnt, BG_PALETTE);

	int i = ly;
	for ITERATE_LINE {
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

		GET_PALETTE_OFFSET_BG_REGULAR;

		PUSH_BG_PIXEL;
	}
}

void PPU::render_affine_bg(int bg, int priority)
{
	GET_BG_PROPERTIES(BG_AFFINE);

	bool affine_wrap = GET_FLAG(bgcnt, BG_OVERFLOW);

	u32 pa = (s32)(s16)io_read<u16>(IO_START + bg*0x10);
	u32 pc = (s32)(s16)io_read<u16>(IO_START + bg*0x10 + 4);

	u32 x2 = ref_x[bg-2];
	u32 y2 = ref_y[bg-2];

	int palette_bank = 0;
	bool color_8 = true;

	int i = ly;
	for ITERATE_LINE {
		if (j >= w)
			break;

		int bx = (s32)x2 >> 8;
		int by = (s32)y2 >> 8;

		x2 += pa;
		y2 += pc;

		if (affine_wrap) {
			bx = (bx % w + w) % w;
			by = (by % h + h) % h;
		}

		if (bx < 0 || bx >= w || by < 0 || by >= h)
			continue;

		int tx = bx / 8;
		int ty = by / 8;

		int px = bx % 8;
		int py = by % 8;

		u32 se_offset = tilemap_base + ty*(w/8) + tx;
		int tile_number = readarr<u8>(vram_data, se_offset);

		GET_PALETTE_OFFSET_BG_AFFINE;

		PUSH_BG_PIXEL;
	}
}

bool PPU::bg_is_enabled(int i)
{
	return dispcnt & (LCD_BG0 * BIT(i));
}

#define ADD_BACKGROUND(x) \
	if (bg_is_enabled((x))) {\
		int priority = GET_FLAG(io_read<u8>(IO_BG0CNT + (x)*2), BG_PRIORITY);\
		bgs_to_render.push_back(std::make_pair(-priority, -(x)));\
	}

#define PRIORITY -x.first
#define BG -x.second

template<u8 mode> void PPU::do_bg_mode()
{
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

	if (bgs_to_render.empty())
		return;

	if constexpr (mode == 0) {
		for (auto x : bgs_to_render) {
			render_text_bg(BG, PRIORITY);
		}
	} else if constexpr (mode == 1) {
		for (auto x : bgs_to_render) {
			if (BG < 2) {
				render_text_bg(BG, PRIORITY);
			} else {
				render_affine_bg(BG, PRIORITY);
			}
		}
	} else if constexpr (mode == 2) {
		for (auto x : bgs_to_render) {
			render_affine_bg(BG, PRIORITY);
		}
	}
}
#undef PRIORITY
#undef BG

template void PPU::do_bg_mode<0>();
template void PPU::do_bg_mode<1>();
template void PPU::do_bg_mode<2>();

#define BITMAP_BG_START \
	int bg = 2;\
	u16 bgcnt = io_read<u16>(IO_BG0CNT + bg*2);\
	int priority = GET_FLAG(bgcnt, BG_PRIORITY);

#define BITMAP_GET_FRAME \
	u8 *p = vram_data;\
	if (dispcnt & LCD_FRAME) {\
		p += 40_KiB;\
	}

void PPU::copy_framebuffer_mode3()
{
	BITMAP_BG_START;

	for ITERATE_SCANLINE {
		u16 color = readarr<u16>(vram_data, i*2);
		bufferA[i] = {color, priority, bg, false, LAYER_BG2, false, false};
	}
}

void PPU::copy_framebuffer_mode4()
{
	BITMAP_BG_START;
	BITMAP_GET_FRAME;

	for ITERATE_SCANLINE {
		u16 color = readarr<u16>(palette_data, p[i]*2);
		bufferA[i] = {color, priority, bg, false, LAYER_BG2, false, false};
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
			u16 color = readarr<u16>(p, (ly*w + j) * 2);
			bufferA[ly*LCD_WIDTH + j] = {color, priority, bg, false, LAYER_BG2, false, false};
		}
	}
}

void PPU::render_sprites()
{
	for (int i = MAX_SPRITES - 1; i >= 0; i--) {
		u16 attr0 = readarr<u16>(oam_data, i*8);

		if (GET_FLAG(attr0, OBJ_DISABLE) && !GET_FLAG(attr0, OBJ_AFFINE)) {
			continue;
		}

		if (GET_FLAG(attr0, OBJ_AFFINE)) {
			render_sprite<true>(i);
		} else {
			render_sprite<false>(i);
		}
	}
}

template <bool is_affine> void PPU::render_sprite(int i)
{
	u16 attr0 = readarr<u16>(oam_data, i*8);
	u16 attr1 = readarr<u16>(oam_data, i*8 + 2);
	u16 attr2 = readarr<u16>(oam_data, i*8 + 4);

	int objx = GET_FLAG(attr1, OBJ_X);
	int objy = GET_FLAG(attr0, OBJ_Y);

	int shape = GET_FLAG(attr0, OBJ_SHAPE);
	int size = GET_FLAG(attr1, OBJ_SIZE);

	int obj_w = OBJ_REGULAR_WIDTH[shape][size];
	int obj_h = OBJ_REGULAR_HEIGHT[shape][size];

	int box_w = obj_w;
	int box_h = obj_h;

	if constexpr (is_affine) {
		if (GET_FLAG(attr0, OBJ_DOUBLESIZE)) {
			box_w *= 2;
			box_h *= 2;
		}
	}

	if (objx >= LCD_WIDTH) {
		objx -= 512;
	}
	if (objy >= LCD_HEIGHT) {
		objy -= 256;
	}

	if (!(objy <= ly && ly < objy + box_h))
		return;

	bool color_8 = GET_FLAG(attr0, OBJ_PALETTE);
	int tile_start = GET_FLAG(attr2, OBJ_TILENUMBER);
	int palette_bank = GET_FLAG(attr2, OBJ_PALETTE_NUMBER);
	int priority = GET_FLAG(attr2, OBJ_PRIORITY);
	int gfx_mode = GET_FLAG(attr0, OBJ_MODE);

	int px0 = obj_w / 2;
	int py0 = obj_h / 2;

	int qx0 = objx + box_w / 2;
	int qy0 = objy + box_h / 2;

	int half_w = box_w / 2;

	int sprite_x, sprite_y;
	u16 x2, y2;
	u16 pa, pc;

	int offset = 0;

	if constexpr (!is_affine) {
		sprite_y = ly - objy;
		if (GET_FLAG(attr1, OBJ_VFLIP)) {
			sprite_y = obj_h - 1 - sprite_y;
		}
		if (GET_FLAG(attr1, OBJ_HFLIP)) {
			sprite_x = obj_w - 1;
			offset = -1;
		} else {
			sprite_x = 0;
			offset = 1;
		}
	} else {
		int affine_index = GET_FLAG(attr1, OBJ_AFFINE_PARAMETER);
		u32 affine_base_addr = 0x20 * affine_index + 6;
		pa = readarr<u16>(oam_data, affine_base_addr);
		u16 pb = readarr<u16>(oam_data, affine_base_addr+8);
		pc = readarr<u16>(oam_data, affine_base_addr+16);
		u16 pd = readarr<u16>(oam_data, affine_base_addr+24);

		x2 = (-half_w)*pa + (ly-qy0)*pb + (px0 << 8);
		y2 = (-half_w)*pc + (ly-qy0)*pd + (py0 << 8);
	}

	for (int j = qx0 - half_w; j < qx0 + half_w; j++, sprite_x += offset) {
		if (j >= LCD_WIDTH)
			break;

		if constexpr (is_affine) {
			sprite_x = (s16)x2 >> 8;
			sprite_y = (s16)y2 >> 8;

			x2 += pa;
			y2 += pc;
		}

		if (j < 0)
			continue;

		if (sprite_y < 0 || sprite_y >= obj_h)
			continue;
		if (sprite_x < 0 || sprite_x >= obj_w)
			continue;

		int tx = sprite_x / 8;
		int px = sprite_x % 8;

		int ty = sprite_y / 8;
		int py = sprite_y % 8;

		int tile_number;

		if (dispcnt & LCD_OBJ_DIM) {
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

		GET_PALETTE_OFFSET_SPRITE;

		u16 color = readarr<u16>(palette_data, 0x200 + palette_offset*2);

		if (palette_number != 0) {
			if (gfx_mode == 0x2) {
				obj_window[j] = true;
			} else {
				pixel_info pixel{color, priority, i, false, LAYER_OBJ, 0, gfx_mode == 1};
				pixel_info other = obj_buffer[ly*LCD_WIDTH + j];

				if (pixel.priority < other.priority || (pixel.priority == other.priority && pixel.bg < other.bg)) {
					obj_buffer[ly*LCD_WIDTH + j] = pixel;
				}
			}

		}
	}
}
template void PPU::render_sprite<true>(int i);
template void PPU::render_sprite<false>(int i);
