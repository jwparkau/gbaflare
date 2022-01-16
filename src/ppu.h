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

enum io_bgcnt_flags {
	BG_PRIORITY	= 0x3,
	BG_MOSAIC	= 0x40,
	BG_PALETTE	= 0x80,
};

enum sreen_entry_flags {
	SE_TILENUMBER 	= BITMASK(10),
	SE_HFLIP	= BIT(10),
	SE_VFLIP	= BIT(11)
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

#define LY() io_data[IO_VCOUNT - IO_START]
#define DISPSTAT() io_data[IO_DISPSTAT - IO_START]
#define LYC() io_data[IO_DISPSTAT - IO_START + 1]

#define LCD_WIDTH 240
#define LCD_HEIGHT 160
constexpr u32 FRAMEBUFFER_SIZE = LCD_WIDTH * LCD_HEIGHT;

#define MAX_SPRITES 128

#define MIN_PRIO 4

struct PPU {
	u16 framebuffer[FRAMEBUFFER_SIZE]{};
	u32 cycles{};
	ppu_modes ppu_mode = PPU_IN_DRAW;

	pixel_info bufferA[FRAMEBUFFER_SIZE]{};
	pixel_info bufferB[FRAMEBUFFER_SIZE]{};
	pixel_info obj_buffer[MAX_SPRITES]{};

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

#endif
