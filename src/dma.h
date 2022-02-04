#ifndef GBAFLARE_DMA_H
#define GBAFLARE_DMA_H

#include "types.h"

#define NUM_DMA 4

#define DMA_DESTCNT_MASK BITMASK(2)
#define DMA_DESTCNT_SHIFT 5
#define DMA_SRCCNT_MASK BITMASK(2)
#define DMA_SRCCNT_SHIFT 7
#define DMA_REPEAT_MASK 1
#define DMA_REPEAT_SHIFT 9
#define DMA_TRANSFER32_MASK 1
#define DMA_TRANSFER32_SHIFT 10
#define DMA_GAMEPAK_DRQ_MASK 1
#define DMA_GAMEPAK_DRQ_SHIFT 11
#define DMA_TRIGGER_MASK BITMASK(2)
#define DMA_TRIGGER_SHIFT 12
#define DMA_SEND_IRQ_MASK 1
#define DMA_SEND_IRQ_SHIFT 14
#define DMA_ENABLED_MASK 1
#define DMA_ENABLED_SHIFT 15

enum dma_triggers {
	DMA_TRIGGER_NOW,
	DMA_TRIGGER_VBLANK,
	DMA_TRIGGER_HBLANK,
	DMA_TRIGGER_SPECIAL
};

enum dma_addr_ctrl {
	DMA_INCREMENT,
	DMA_DECREMENT,
	DMA_FIXED,
	DMA_RELOAD
};

enum dma_channels {
	DMA_0	= 0x1,
	DMA_1	= 0x2,
	DMA_2	= 0x4,
	DMA_3	= 0x8
};

struct dma_transfer {
	u32 sad;
	u32 dad;
	u32 cnt_l;
	u32 cnt_h;
	u32 count;

	bool enabled()
	{
		return GET_FLAG(cnt_h, DMA_ENABLED);
	}

	bool is_special()
	{
		return GET_FLAG(cnt_h, DMA_TRIGGER) == DMA_TRIGGER_SPECIAL;
	}
};

struct DMA {
	int channel{};

	dma_transfer transfers[NUM_DMA]{};
	u32 last_value[NUM_DMA]{};

	u32 dma_enabled{};
	u32 dma_active{};

	void step();
	void step_channel(int ch);

	void on_write(addr_t addr, u8 old_value, u8 new_value);
	void on_hblank();
	void on_vblank();

	u16 get_cnt_h(int ch);
	u32 load_cnt_l(int ch);
	u32 load_dad(int ch);

	void icycle();

	DECLARE_READ_WRITE;
};

extern DMA dma;

#endif
