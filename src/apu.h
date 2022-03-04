#ifndef GBAFLARE_APU_H
#define GBAFLARE_APU_H

#include "types.h"

#define FIFO_SIZE 33

#define FIFO_A 0
#define FIFO_B 1
#define NUM_FIFOS 2

#define CYCLES_PER_FS_TICK 32768

#define PSG_RIGHT_VOL_MASK BITMASK(3)
#define PSG_RIGHT_VOL_SHIFT 0
#define PSG_LEFT_VOL_MASK BITMASK(3)
#define PSG_LEFT_VOL_SHIFT 4

enum soundcnt_l_flags {
	SOUND1_RIGHT_ON = 0x100,
	SOUND2_RIGHT_ON = 0x200,
	SOUND3_RIGHT_ON = 0x400,
	SOUND4_RIGHT_ON = 0x800,
	SOUND1_LEFT_ON = 0x1000,
	SOUND2_LEFT_ON = 0x2000,
	SOUND3_LEFT_ON = 0x4000,
	SOUND4_LEFT_ON = 0x8000
};

enum soundcnt_h_flags {
	PSG_VOL = 0x3,
	DMA_A_VOL = 0x4,
	DMA_B_VOL = 0x8,
	DMA_A_RIGHT = 0x100,
	DMA_A_LEFT = 0x200,
	DMA_A_TIMER = 0x400,
	DMA_A_RESET = 0x800
};

enum soundcnt_x_flags {
	SOUND1_ON = 0x1,
	SOUND2_ON = 0x2,
	SOUND3_ON = 0x4,
	SOUND4_ON = 0x8,
	SOUND_MASTER_ENABLE = 0x80
};

struct FIFO {
	int start{};
	int end{};
	int size{};
	u8 buffer[FIFO_SIZE]{};

	void reset();
	void enqueue8(u8 x);
	void enqueue32(u32 x);
	u8 dequeue();

	void step();
};


struct APU {
	u32 channel_cycles{};
	u32 sample_cycles{};
	u32 frameseq_cycles{};
	u32 frame_sequencer{};
	s8 fifo_v[NUM_FIFOS]{};

	void reset();
	void channel_step();
	void step();
	void on_timer_overflow(int i);
	void on_write(addr_t addr, u8 old_value, u8 new_value);

	void clock_length();
	void clock_sweep();
	void clock_envelope();
};

extern FIFO fifos[NUM_FIFOS];
extern APU apu;

#endif
