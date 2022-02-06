#ifndef GBAFLARE_APU_H
#define GBAFLARE_APU_H

#include "types.h"

#define FIFO_SIZE 33

#define FIFO_A 0
#define FIFO_B 1

enum soundcnt_h_flags {
	DMA_A_VOL = 0x4,
	DMA_B_VOL = 0x8,
	DMA_A_RIGHT = 0x100,
	DMA_A_LEFT = 0x200,
	DMA_A_TIMER = 0x400,
	DMA_A_RESET = 0x800
};

enum soundcnt_x_flags {
	SOUND_MASTER_ENABLE = 0x80
};

struct FIFO {
	int start{};
	int end{};
	int size{};
	u8 buffer[FIFO_SIZE];

	void reset();
	void enqueue8(u8 x);
	void enqueue32(u32 x);
	u8 dequeue();

	void step();
};

extern FIFO fifos[2];

struct APU {
	u32 cycles{};
	u32 last{};
	s8 fifo_v[2]{};

	void step();
	void on_timer_overflow(int i);
	void on_write(addr_t addr, u8 old_value, u8 new_value);
};

extern APU apu;

#endif
