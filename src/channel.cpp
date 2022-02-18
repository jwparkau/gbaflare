#include "channel.h"
#include "memory.h"
#include "scheduler.h"

const int WAVE_DUTY_TABLE[4][8] = {
	{0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, 1, 1},
	{0, 0, 0, 0, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 0, 0}
};

const addr_t ENVELOPE_ADDR[4] = {IO_SOUND1CNT_H, IO_SOUND2CNT_L, 0, IO_SOUND4CNT_L};

ChannelState channel_states[NUM_PSG_CHANNELS];
SweepState sweep_state;
NoiseState noise_state;
WaveState wave_state;

template<int ch> void set_freq_timer()
{
	auto &state = channel_states[ch-1];

	if constexpr (ch == 1) {
		state.freq_timer = 16 * (2048 - GET_FLAG(io_read<u16>(IO_SOUND1CNT_X), SOUND_FREQ));
	} else if constexpr (ch == 2) {
		state.freq_timer = 16 * (2048 - GET_FLAG(io_read<u16>(IO_SOUND2CNT_H), SOUND_FREQ));
	} else if constexpr (ch == 3) {
		state.freq_timer = 8 * (2048 - GET_FLAG(io_read<u16>(IO_SOUND3CNT_X), SOUND_FREQ));
	} else {
		u16 cnt = io_read<u16>(IO_SOUND4CNT_H);
		int div = GET_FLAG(cnt, NOISE_DIV);
		int shift = GET_FLAG(cnt, NOISE_SHIFT);

		state.freq_timer = (div == 0 ? 32 : 64 * div) << shift;
	}
}

template<int ch> void step_psg_channel()
{
	auto &state = channel_states[ch-1];

	state.freq_timer -= elapsed;

	if (state.freq_timer <= 0) {
		int surplus = -state.freq_timer;

		set_freq_timer<ch>();
		state.freq_timer += surplus;
		state.wave_pos++;

		if constexpr (ch == 4) {
			u16 &lfsr = noise_state.LFSR;
			int xor_result = (lfsr & 1) ^ (lfsr >> 1 & 1);
			lfsr = (lfsr >> 1) | (xor_result << 14);

			if (GET_FLAG(io_read<u8>(IO_SOUND4CNT_H), NOISE_WIDTH)) {
				lfsr &= ~(1 << 6);
				lfsr |= xor_result << 6;
			}
		}
	}

	schedule_after(state.freq_timer);
}

template<int ch> int get_psg_value()
{
	auto &state = channel_states[ch-1];

	int value;

	if constexpr (ch == 1) {
		value = state.current_volume * WAVE_DUTY_TABLE[GET_FLAG(io_read<u8>(IO_SOUND1CNT_H), SOUND_DUTY)][state.wave_pos %= 8];
	} else if constexpr (ch == 2) {
		value = state.current_volume * WAVE_DUTY_TABLE[GET_FLAG(io_read<u8>(IO_SOUND2CNT_L), SOUND_DUTY)][state.wave_pos %= 8];
	} else if constexpr (ch == 3) {
		u8 cntl = io_read<u8>(IO_SOUND3CNT_L);
		if (GET_FLAG(cntl, WAVE_CH3_ENABLED)) {
			state.wave_pos %= 32;
			u8 sample = wave_ram[WAVE_BANK()][state.wave_pos / 2];
			if (state.wave_pos % 2 == 0) {
				value = sample >> 4 & BITMASK(4);
			} else {
				value = sample & BITMASK(4);
			}

			if (state.wave_pos == 31) {
				if (GET_FLAG(cntl, WAVE_DIM)) {
					io_data[IO_SOUND3CNT_L - IO_START] ^= BIT(6);
				}
			}
		} else {
			value = 0;
		}
	} else {
		value = state.current_volume * (1 - (noise_state.LFSR & 1));
	}

	return value;
}

template<int ch> void clock_envelope_ch()
{
	auto &state = channel_states[ch-1];

	u16 cnt = io_read<u16>(ENVELOPE_ADDR[ch-1]);
	int period = GET_FLAG(cnt, ENV_TIME);

	if (period != 0) {
		if (state.period_timer > 0) {
			state.period_timer--;
		}

		if (state.period_timer == 0) {
			state.period_timer = period;

			int increase = GET_FLAG(cnt, ENV_DIR);
			if (increase && state.current_volume < 0xF) {
				state.current_volume++;
			} else if (!increase && state.current_volume > 0) {
				state.current_volume--;
			}
		}
	}
}

template<int ch> void disable_ch()
{
	io_data[IO_SOUNDCNT_X - IO_START] &= ~BIT(ch-1);
	channel_states[ch-1].enabled = false;
}

template<int ch> void enable_ch()
{
	io_data[IO_SOUNDCNT_X - IO_START] |= BIT(ch-1);
	channel_states[ch-1].enabled = true;
}

template<int ch> void load_length_timer(u8 new_value)
{
	if constexpr (ch == 3) {
		channel_states[2].length_timer = 256 - new_value;
	} else {
		channel_states[ch-1].length_timer = 64 - (new_value & BITMASK(6));
	}
}

template<int ch> bool length_enabled()
{
	addr_t addr;
	if constexpr (ch == 1) {
		addr = IO_SOUND1CNT_X;
	} else if constexpr (ch == 2) {
		addr = IO_SOUND2CNT_H;
	} else if constexpr (ch == 3) {
		addr = IO_SOUND3CNT_X;
	} else {
		addr = IO_SOUND4CNT_H;
	}

	return GET_FLAG(io_read<u16>(addr), LENGTH_ENABLED);
}

template<int ch> void clock_length_ch()
{
	auto &state = channel_states[ch-1];

	if (length_enabled<ch>()) {
		state.length_timer--;

		if (state.length_timer == 0) {
			disable_ch<ch>();
		}
	}
}

template<int ch> void trigger_ch()
{
	auto &state = channel_states[ch-1];

	enable_ch<ch>();

	if constexpr (ch == 1 || ch == 2 || ch == 4) {
		u16 envcnt = io_read<u16>(ENVELOPE_ADDR[ch-1]);
		state.period_timer = GET_FLAG(envcnt, ENV_TIME);
		state.current_volume = GET_FLAG(envcnt, ENV_VOL);
	}

	if constexpr (ch == 1) {
		sweep_state.on_trigger();
	}

	if (state.length_timer == 0) {
		if constexpr (ch == 3) {
			state.length_timer = 256;
		} else {
			state.length_timer = 64;
		}
	}

	if constexpr (ch == 4) {
		noise_state.LFSR = 0x7FFF;
	}

	state.wave_pos = 0;
	set_freq_timer<ch>();
}

int SweepState::calculate_freq()
{
	u8 cnt = io_read<u8>(IO_SOUND1CNT_L);
	int shift = GET_FLAG(cnt, SWEEP_NUM);
	int decrease = GET_FLAG(cnt, SWEEP_DIR);

	int new_freq = shadow_freq >> shift;

	if (decrease) {
		new_freq = shadow_freq - new_freq;
	} else {
		new_freq = shadow_freq + new_freq;
	}

	if (new_freq >= 2048) {
		disable_ch<1>();
	}

	return new_freq;
}

void SweepState::do_sweep_clock()
{
	if (sweep_timer > 0) {
		sweep_timer--;

		if (sweep_timer == 0) {
			u8 cnt = io_read<u8>(IO_SOUND1CNT_L);
			int period = GET_FLAG(cnt, SWEEP_TIME);
			sweep_timer = period != 0 ? period : 8;

			if (sweep_enabled && period > 0) {
				int new_freq = calculate_freq();

				if (new_freq < 2048 && GET_FLAG(cnt, SWEEP_NUM) > 0) {
					shadow_freq = new_freq;
					u16 freqcnt = io_read<u16>(IO_SOUND1CNT_X);
					SET_FLAG(freqcnt, SOUND_FREQ, new_freq);
					io_write<u16>(IO_SOUND1CNT_X, freqcnt);
					set_freq_timer<1>();
					schedule_after(channel_states[0].freq_timer);

					calculate_freq();
				}
			}
		}
	}
}

void SweepState::on_trigger()
{
	shadow_freq = channel_states[0].freq_timer;

	u8 cnt = io_read<u8>(IO_SOUND1CNT_L);
	int shift = GET_FLAG(cnt, SWEEP_NUM);
	int period = GET_FLAG(cnt, SWEEP_TIME);

	sweep_timer = period != 0 ? period : 8;
	sweep_enabled = period != 0 || shift != 0;

	if (shift != 0) {
		calculate_freq();
	}
}

int get_psg_value(int ch)
{
	if (ch == 1) {
		return get_psg_value<1>();
	} else if (ch == 2) {
		return get_psg_value<2>();
	} else if (ch == 3) {
		return get_psg_value<3>();
	} else {
		return get_psg_value<4>();
	}
}

void step_psg(int ch)
{
	if (ch == 1) {
		step_psg_channel<1>();
	} else if (ch == 2) {
		step_psg_channel<2>();
	} else if (ch == 3) {
		step_psg_channel<3>();
	} else {
		step_psg_channel<4>();
	}
}

void psg_clock_length(int ch)
{
	if (ch == 1) {
		clock_length_ch<1>();
	} else if (ch == 2) {
		clock_length_ch<2>();
	} else if (ch == 3) {
		clock_length_ch<3>();
	} else {
		clock_length_ch<4>();
	}
}

void psg_clock_envelope(int ch)
{
	if (ch == 1) {
		clock_envelope_ch<1>();
	} else if (ch == 2) {
		clock_envelope_ch<2>();
	} else {
		clock_envelope_ch<4>();
	}
}

void psg_trigger_ch(int ch)
{
	if (ch == 1) {
		trigger_ch<1>();
	} else if (ch == 2) {
		trigger_ch<2>();
	} else if (ch == 3) {
		trigger_ch<3>();
	} else {
		trigger_ch<4>();
	}
}

void psg_load_length_timer(int ch, u8 new_value)
{
	if (ch == 1) {
		load_length_timer<1>(new_value);
	} else if (ch == 2) {
		load_length_timer<2>(new_value);
	} else if (ch == 3) {
		load_length_timer<3>(new_value);
	} else {
		load_length_timer<4>(new_value);
	}
}
