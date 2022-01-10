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
	LCD_VBLANK	= 0x1,
	LCD_HBLANK	= 0x2,
	LCD_VCOUNTER	= 0x4,
	LCD_VBLANK_IRQ	= 0x8,
	LCD_HBLANK_IRQ	= 0x10,
	LCD_VOUNTER_IRQ	= 0x20
};

enum ppu_modes {
	PPU_IN_DRAW,
	PPU_IN_HBLANK,
	PPU_IN_VBLANK
};

#define LY() io_data[IO_VCOUNT - IO_START]
#define DISPSTAT() io_data[IO_DISPSTAT - IO_START]
#define LYC() io_data[IO_DISPSTAT - IO_START + 1]

struct PPU {
	u16 framebuffer[240 * 160];
	u32 cycles{};
	ppu_modes ppu_mode = PPU_IN_DRAW;

	void step();

	void on_vblank();
	void copy_framebuffer_mode4();
};

extern PPU ppu;

#endif
