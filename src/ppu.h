#ifndef GBAFLARE_PPU_H
#define GBAFLARE_PPU_H

#include "types.h"
#include "platform.h"

enum io_dispcnt_flags {
	LCD_RESERVED	= 0x8,
	LCD_FRAME	= 0x10,
	LCD_INTERVAL	= 0x20,
	LCD_OBJ_DIM	= 0x40,
	LCD_FORCE_BLANK	= 0x80,
	LCD_BG0		= 0x100,
	LCD_BG1		= 0x200,
	LCD_BG2		= 0x400,
	LCD_BG3		= 0x800,
	LCD_OBJ		= 0x1000,
	LCD_WINDOW0	= 0x2000,
	LCD_WINDOW1	= 0x4000,
	LCD_OBJWINDOW	= 0x8000
};
#define LCD_BGMODE_MASK BITMASK(3)
#define LCD_BGMODE_SHIFT 0

enum io_dispstat_flags {
	LCD_VBLANK	 = 0x1,
	LCD_HBLANK	 = 0x2,
	LCD_VCOUNTER	 = 0x4,
	LCD_VBLANK_IRQ	 = 0x8,
	LCD_HBLANK_IRQ	 = 0x10,
	LCD_VCOUNTER_IRQ = 0x20
};

enum ppu_modes {
	PPU_IN_DRAW,
	PPU_IN_HBLANK,
	PPU_IN_VBLANK_1,
	PPU_IN_VBLANK_2
};

enum render_layers {
	LAYER_BG0,
	LAYER_BG1,
	LAYER_BG2,
	LAYER_BG3,
	LAYER_OBJ,
	LAYER_BD
};

enum blend_effects {
	BLEND_NONE,
	BLEND_ALPHA,
	BLEND_INC,
	BLEND_DEC
};

#define BG_PRIORITY_MASK BITMASK(2)
#define BG_PRIORITY_SHIFT 0
#define BG_CBB_MASK BITMASK(2)
#define BG_CBB_SHIFT 2
#define BG_MOSAIC_MASK 1
#define BG_MOSAIC_SHIFT 6
#define BG_PALETTE_MASK 1
#define BG_PALETTE_SHIFT 7
#define BG_SBB_MASK BITMASK(5)
#define BG_SBB_SHIFT 8
#define BG_OVERFLOW_MASK 1
#define BG_OVERFLOW_SHIFT 13
#define BG_SCREEN_SIZE_MASK BITMASK(2)
#define BG_SCREEN_SIZE_SHIFT 14

#define SE_TILENUMBER_MASK BITMASK(10)
#define SE_TILENUMBER_SHIFT 0
#define SE_HFLIP_MASK 1
#define SE_HFLIP_SHIFT 10
#define SE_VFLIP_MASK 1
#define SE_VFLIP_SHIFT 11
#define SE_PALETTE_NUMBER_MASK BITMASK(4)
#define SE_PALETTE_NUMBER_SHIFT 12

#define OBJ_Y_MASK BITMASK(8)
#define OBJ_Y_SHIFT 0
#define OBJ_AFFINE_MASK 1
#define OBJ_AFFINE_SHIFT 8
#define OBJ_DISABLE_MASK 1
#define OBJ_DISABLE_SHIFT 9
#define OBJ_DOUBLESIZE_MASK 1
#define OBJ_DOUBLESIZE_SHIFT 9
#define OBJ_MODE_MASK BITMASK(2)
#define OBJ_MODE_SHIFT 10
#define OBJ_MOSAIC_MASK 1
#define OBJ_MOSAIC_SHIFT 12
#define OBJ_PALETTE_MASK 1
#define OBJ_PALETTE_SHIFT 13
#define OBJ_SHAPE_MASK BITMASK(2)
#define OBJ_SHAPE_SHIFT 14

#define OBJ_X_MASK BITMASK(9)
#define OBJ_X_SHIFT 0
#define OBJ_AFFINE_PARAMETER_MASK BITMASK(5)
#define OBJ_AFFINE_PARAMETER_SHIFT 9
#define OBJ_HFLIP_MASK 1
#define OBJ_HFLIP_SHIFT 12
#define OBJ_VFLIP_MASK 1
#define OBJ_VFLIP_SHIFT 13
#define OBJ_SIZE_MASK BITMASK(2)
#define OBJ_SIZE_SHIFT 14

#define OBJ_TILENUMBER_MASK BITMASK(10)
#define OBJ_TILENUMBER_SHIFT 0
#define OBJ_PRIORITY_MASK BITMASK(2)
#define OBJ_PRIORITY_SHIFT 10
#define OBJ_PALETTE_NUMBER_MASK BITMASK(4)
#define OBJ_PALETTE_NUMBER_SHIFT 12

#define WINDOW_RIGHT_MASK BITMASK(8)
#define WINDOW_RIGHT_SHIFT 0
#define WINDOW_BOTTOM_MASK BITMASK(8)
#define WINDOW_BOTTOM_SHIFT 0
#define WINDOW_LEFT_MASK BITMASK(8)
#define WINDOW_LEFT_SHIFT 8
#define WINDOW_TOP_MASK BITMASK(8)
#define WINDOW_TOP_SHIFT 8

#define BLEND_MODE_MASK BITMASK(2)
#define BLEND_MODE_SHIFT 6

#define BLEND_EVA_MASK BITMASK(5)
#define BLEND_EVA_SHIFT 0
#define BLEND_EVB_MASK BITMASK(5)
#define BLEND_EVB_SHIFT 8
#define BLEND_EVY_MASK BITMASK(5)
#define BLEND_EVY_SHIFT 0

#define COLOR_R_MASK BITMASK(5)
#define COLOR_R_SHIFT 0
#define COLOR_G_MASK BITMASK(5)
#define COLOR_G_SHIFT 5
#define COLOR_B_MASK BITMASK(5)
#define COLOR_B_SHIFT 10

#define MOSAIC_BH_MASK BITMASK(4)
#define MOSAIC_BH_SHIFT 0
#define MOSAIC_BV_MASK BITMASK(4)
#define MOSAIC_BV_SHIFT 4
#define MOSAIC_OBJH_MASK BITMASK(4)
#define MOSAIC_OBJH_SHIFT 8
#define MOSAIC_OBJV_MASK BITMASK(4)
#define MOSAIC_OBJV_SHIFT 12


#define LY() io_data[IO_VCOUNT - IO_START]
#define DISPSTAT() io_data[IO_DISPSTAT - IO_START]
#define LYC() io_data[IO_DISPSTAT - IO_START + 1]
#define DISPCNT() io_data[IO_DISPCNT - IO_START]


#define MAX_SPRITES 128

#define MIN_PRIO 4
#define NOT_BG 0

#define ITERATE_SCANLINE (u32 i = ly*LCD_WIDTH, j = 0; j < LCD_WIDTH; i++, j++)
#define ITERATE_LINE (int j = 0; j < LCD_WIDTH; j++)

struct window_info {
	int l;
	int r;
	int t;
	int b;
	bool enabled;
	bool y_in_window;
};

struct pixel_info {
	u16 color;
	int priority;
	int bg;
	bool is_transparent;
	int layer;
	int enable_blending;
	bool force_alpha;
};

struct PPU {
	u32 cycles{};
	ppu_modes ppu_mode = PPU_IN_DRAW;

	int ly{};
	u16 dispcnt{};
	bool vblank{};

	pixel_info bufferA[FRAMEBUFFER_SIZE]{};
	pixel_info bufferB[FRAMEBUFFER_SIZE]{};
	pixel_info obj_buffer[FRAMEBUFFER_SIZE]{};

	window_info windows[2]{};
	bool objwindow_enabled{};
	bool winout_enabled{};
	bool obj_window[LCD_WIDTH]{};

	u32 ref_x[2]{};
	u32 ref_y[2]{};

	void step();
	void on_vblank();
	void on_hblank();

	void draw_scanline();

	template<u8 mode> void do_bg_mode();
	void render_text_bg(int bg, int priority);
	void render_affine_bg(int bg, int priority);
	void copy_framebuffer_mode3();
	void copy_framebuffer_mode4();
	void copy_framebuffer_mode5();

	void render_sprites();
	template<bool is_affine> void render_sprite(int i);

	bool bg_is_enabled(int i);
	bool should_push_pixel(int bg, int x, int &blend);
	void check_window(int n, int x);
	void copy_affine_ref();
	void setup_windows();
	void setup_window(int n);
};

extern PPU ppu;

#endif
