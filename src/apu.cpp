#include "apu.h"
#include "platform.h"
#include "memory.h"
#include "dma.h"
#include "channel.h"

const int psg_volume_div[4] = {4, 2, 1, 1};
const int wave_volume_factor[4] = {0, 4, 2, 1};

FIFO fifos[NUM_FIFOS];
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
	sample_cycles += elapsed;

	if (sample_cycles >= CYCLES_PER_SAMPLE) {
		sample_cycles -= CYCLES_PER_SAMPLE;

		s16 left = 0;
		s16 right = 0;

		s16 psg_left = 0;
		s16 psg_right = 0;

		u16 soundcnt_l = io_read<u16>(IO_SOUNDCNT_L);
		u16 soundcnt_x = io_read<u16>(IO_SOUNDCNT_X);
		u16 soundcnt_h = io_read<u16>(IO_SOUNDCNT_H);
		if (soundcnt_x & SOUND_MASTER_ENABLE) {
			for (int i = 0; i < NUM_FIFOS; i++) {
				int v = fifo_v[i] * 4;
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

			for (int ch = 1; ch <= NUM_PSG_CHANNELS; ch++) {
				if (channel_states[ch-1].enabled) {
					int v = get_psg_value(ch) * 16;
					v = v / psg_volume_div[soundcnt_h & PSG_VOL];

					if (ch == 3) {
						u16 cnt3 = io_read<u16>(IO_SOUND3CNT_H);
						if (GET_FLAG(cnt3, WAVE_FORCE)) {
							v = v * 3 / 4;
						} else {
							v = v * wave_volume_factor[GET_FLAG(cnt3, WAVE_VOL)] / 4;
						}
					}

					if (soundcnt_l & (SOUND1_RIGHT_ON * BIT(ch-1))) {
						psg_right += v;
					}
					if (soundcnt_l & (SOUND1_LEFT_ON * BIT(ch-1))) {
						psg_left += v;
					}

				}
			}
		}

		int k = 20;

		psg_left = psg_left * GET_FLAG(soundcnt_l, PSG_LEFT_VOL) / 7;
		psg_right = psg_right * GET_FLAG(soundcnt_l, PSG_RIGHT_VOL) / 7;


		left += psg_left;
		right += psg_right;

		audiobuffer[audio_buffer_index++] = left*k;
		audiobuffer[audio_buffer_index++] = right*k;
	}

	schedule_after(CYCLES_PER_SAMPLE - sample_cycles);
}

void APU::channel_step()
{
	frameseq_cycles += elapsed;
	if (frameseq_cycles >= CYCLES_PER_FS_TICK) {
		frameseq_cycles -= CYCLES_PER_FS_TICK;

		frame_sequencer = (frame_sequencer + 1) % 8;

		if (frame_sequencer % 2 == 0) {
			clock_length();
		}

		if (frame_sequencer == 7) {
			clock_envelope();
		}

		if (frame_sequencer == 2 || frame_sequencer == 6) {
			clock_sweep();
		}
	}

	schedule_after(CYCLES_PER_FS_TICK - frameseq_cycles);

	step_psg(1);
	step_psg(2);
	step_psg(3);
	step_psg(4);
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

void APU::clock_length()
{
	psg_clock_length(1);
	psg_clock_length(2);
	psg_clock_length(3);
	psg_clock_length(4);
}

void APU::clock_sweep()
{
	sweep_state.do_sweep_clock();
}

void APU::clock_envelope()
{
	psg_clock_envelope(1);
	psg_clock_envelope(2);
	psg_clock_envelope(4);
}
