#include "apu.h"
#include "platform.h"
#include "memory.h"
#include "dma.h"

FIFO fifos[2];

APU apu;

void FIFO::reset()
{
	start = end = size = 0;
	for (int i = 0; i < FIFO_SIZE; i++) {
		buffer[i] = 0;
	}
}

void FIFO::enqueue32(u32 x)
{
	enqueue8(x);
	x >>= 8;
	enqueue8(x);
	x >>= 8;
	enqueue8(x);
	x >>= 8;
	enqueue8(x);
}

void FIFO::enqueue8(u8 x)
{
	buffer[end] = x;
	end = (end + 1) % FIFO_SIZE;
	size++;
}

u8 FIFO::dequeue()
{
	u8 x = buffer[start];
	start = (start + 1) % FIFO_SIZE;
	size--;

	return x;
}

void APU::step()
{
	cycles += elapsed;
	if (cycles >= 512) {
		cycles -= 512;
	} else {
		return;
	}

	s16 left = 0;
	s16 right = 0;

	u16 soundcnt_x = io_read<u16>(IO_SOUNDCNT_X);
	u16 soundcnt_h = io_read<u16>(IO_SOUNDCNT_H);
	if (soundcnt_x & SOUND_MASTER_ENABLE) {
		for (int i = 0; i < 2; i++) {
			int v = fifo_v[i] * 4; // 8 -> 10 bit range;
			if (soundcnt_h & (DMA_A_VOL * BIT(i))) {
				v /= 2;
			}

			if (soundcnt_h & (DMA_A_LEFT * BIT(i*4))) {
				left += v;
			}

			if (soundcnt_h & (DMA_A_RIGHT * BIT(i*4))) {
				right += v;
			}
		}
	}

	int k = 20;

	audiobuffer[audio_buffer_index++] = left*k;
	audiobuffer[audio_buffer_index++] = right*k;
	schedule_after(512 - cycles);
}

void APU::on_timer_overflow(int i)
{
	u16 soundcnt_h = io_read<u16>(IO_SOUNDCNT_H);
	for (int k = 0; k < 2; k++) {
		if ((bool)(soundcnt_h & (DMA_A_TIMER * BIT(k*4))) == i) {
			fifo_v[k] = fifos[k].dequeue();
			if (fifos[k].size <= 16 && dma.transfers[k+1].enabled() && dma.transfers[k+1].is_special()) {
				dma.dma_active |= BIT(k+1);
			}
		}
	}
}

void APU::on_write(addr_t addr, u8 old_value, u8 new_value)
{
	if (addr == IO_SOUNDCNT_H + 1) {
		if (new_value & BIT(3)) {
			fifos[0].reset();
		}
		if (new_value & BIT(7)) {
			fifos[1].reset();
		}
	}
}
