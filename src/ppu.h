#ifndef GBAFLARE_PPU_H
#define GBAFLARE_PPU_H

#include "types.h"

enum io_dispcnt_flags {
	LCD_BGMODE 	= 0x7,
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
	PPU_IN_VBLANK
};

enum render_layers {
	LAYER_BG0,
	LAYER_BG1,
	LAYER_BG2,
	LAYER_BG3,
	LAYER_OBJ,
	LAYER_BD
};

struct pixel_info {
	u16 color_555;
	int priority;
	int bg;
	bool is_transparent;
	int layer;
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
#define BG_OVERFLOW 1
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

#define GET_FLAG(x, f) ((x) >> f##_SHIFT & f##_MASK)

#define LY() io_data[IO_VCOUNT - IO_START]
#define DISPSTAT() io_data[IO_DISPSTAT - IO_START]
#define LYC() io_data[IO_DISPSTAT - IO_START + 1]
#define DISPCNT() io_data[IO_DISPCNT - IO_START]

#define LCD_WIDTH 240l
#define LCD_HEIGHT 160l
#define FRAMEBUFFER_SIZE LCD_WIDTH * LCD_HEIGHT

#define MAX_SPRITES 128

#define MIN_PRIO 4

struct PPU {
	u16 framebuffer[FRAMEBUFFER_SIZE]{};
	u32 cycles{};
	ppu_modes ppu_mode = PPU_IN_DRAW;

	pixel_info bufferA[FRAMEBUFFER_SIZE]{};
	pixel_info bufferB[FRAMEBUFFER_SIZE]{};
	pixel_info obj_buffer[FRAMEBUFFER_SIZE]{};

	void step();

	void draw_scanline();
	void on_vblank();
	void do_bg_mode0();
	void render_text_bg(int bg, int priority);
	void copy_framebuffer_mode3();
	void copy_framebuffer_mode4();
	void copy_framebuffer_mode5();
	void render_sprites();

	bool bg_is_enabled(int i);
};

extern PPU ppu;

extern int vblank_flag;

#endif
