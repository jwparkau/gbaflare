#include "dma.h"
#include "memory.h"
#include "scheduler.h"
#include "apu.h"

#include <bit>
#include <iostream>

static const int sad_offset[4] = {1, -1, 0, 1}; // last value is invalid but dont want to segfault
static const int dad_offset[4] = {1, -1, 0, 1};

DMA dma;

void DMA::step()
{
	channel = std::countr_zero(dma_active);

	if (channel < NUM_DMA) {
		step_channel(channel);
	}
}

void DMA::step_channel(int ch)
{
	auto &t = transfers[ch];

	u32 x;
	int width = GET_FLAG(t.cnt_h, DMA_TRANSFER32) ? 4 : 2;

	bool first = t.count == 0;
	if (first && GET_FLAG(t.cnt_h, DMA_TRIGGER) == DMA_TRIGGER_NOW) {
		cpu_cycles += 2;
	}

	if (width == 4) {
		if (t.sad < 0x0200'0000) {
			swrite32(t.dad, last_value[ch]);
		} else {
			if (first) {
				x = nread32(t.sad);
			} else {
				x = sread32(t.sad);
			}
			swrite32(t.dad, x);
			last_value[ch] = x;
		}
	} else {
		if (t.sad < 0x0200'0000) {
			swrite16(t.dad, last_value[ch]);
		} else {
			if (first) {
				x = nread16(t.sad);
			} else {
				x = sread16(t.sad);
			}
			swrite16(t.dad, x);
			last_value[ch] = x * 0x10001;
		}
	}

	int destcnt = GET_FLAG(t.cnt_h, DMA_DESTCNT);
	int sad_delta = sad_offset[GET_FLAG(t.cnt_h, DMA_SRCCNT)];
	if (t.sad >= 0x0800'0000 && t.sad < 0x0E00'0000) {
		sad_delta = 1;
	}

	t.sad += width * sad_delta;
	t.dad += width * dad_offset[destcnt];

	t.count++;

	if (t.count >= t.cnt_l) {
		t.count = 0;
		if (GET_FLAG(t.cnt_h, DMA_SEND_IRQ)) {
			request_interrupt(IRQ_DMA0 * BIT(ch));
		}

		if (GET_FLAG(t.cnt_h, DMA_REPEAT) && GET_FLAG(t.cnt_h, DMA_TRIGGER) != DMA_TRIGGER_NOW) {
			t.cnt_l = load_cnt_l(ch);
			if (destcnt == 3) {
				t.dad = align(load_dad(ch), width);
			}
			if (t.is_special() && (ch == 1 || ch == 2)) {
				t.cnt_l = 4;
			}
		} else {
			io_data[IO_DMA0CNT_H - IO_START + ch*12 + 1] &= ~0x80;
		}

		dma_active &= ~BIT(ch);
	}
}

u32 DMA::load_cnt_l(int ch)
{
	u32 cnt_l = io_read<u16>(IO_DMA0CNT_L + ch*12) & BITMASK(16);
	if (ch == 3) {
		if (cnt_l == 0) {
			cnt_l = 0x10000;
		}
	} else {
		cnt_l &= BITMASK(14);
		if (cnt_l == 0) {
			cnt_l = 0x4000;
		}
	}

	return cnt_l;
}

u32 DMA::load_dad(int ch)
{
	u32 dad = io_read<u32>(IO_DMA0DAD + ch*12);
	if (ch == 3) {
		dad &= BITMASK(28);
	} else {
		dad &= BITMASK(27);
	}

	return dad;
}

void DMA::on_write(addr_t addr, u8 old_value, u8 new_value)
{
	int ch = (addr - IO_DMA0CNT_H - 1) / 12;
	u16 cnt_h = new_value << 8 | io_read<u8>(IO_DMA0CNT_H + ch*12);

	if (GET_FLAG(new_value << 8, DMA_ENABLED) && !GET_FLAG(old_value << 8, DMA_ENABLED)) {
		u32 sad = io_read<u32>(IO_DMA0SAD + ch*12);
		u32 dad = load_dad(ch);
		u32 cnt_l = load_cnt_l(ch);

		if (ch == 0) {
			sad &= BITMASK(27);
		} else {
			sad &= BITMASK(28);
		}

		int width = GET_FLAG(cnt_h, DMA_TRANSFER32) ? 4 : 2;
		int trigger = GET_FLAG(cnt_h, DMA_TRIGGER);
		if (trigger == DMA_TRIGGER_SPECIAL && (ch == 1 || ch == 2)) {
			width = 4;
			cnt_l = 4;
			SET_FLAG(cnt_h, DMA_DESTCNT, DMA_FIXED);
		}
		dad = align(dad, width);
		sad = align(sad, width);

		transfers[ch] = {sad, dad, cnt_l, cnt_h, 0};

		if (trigger == DMA_TRIGGER_NOW) {
			dma_active |= BIT(ch);
		}
	} else if (!GET_FLAG(new_value << 8, DMA_ENABLED) && GET_FLAG(old_value << 8, DMA_ENABLED)) {
		dma_active &= ~BIT(ch);
	}
}

void DMA::on_vblank()
{
	for (int ch = 0; ch < NUM_DMA; ch++) {
		u16 cnt_h = get_cnt_h(ch);
		if (GET_FLAG(cnt_h, DMA_ENABLED) && GET_FLAG(cnt_h, DMA_TRIGGER) == 1) {
			dma_active |= BIT(ch);
		}
	}
}

void DMA::on_hblank()
{
	for (int ch = 0; ch < NUM_DMA; ch++) {
		u16 cnt_h = get_cnt_h(ch);
		if (GET_FLAG(cnt_h, DMA_ENABLED) && GET_FLAG(cnt_h, DMA_TRIGGER) == 2) {
			dma_active |= BIT(ch);
		}
	}
}

u16 DMA::get_cnt_h(int ch)
{
	return io_read<u16>(IO_DMA0CNT_H + ch*12);
}

DEFINE_READ_WRITE(DMA)
