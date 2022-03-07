#ifndef GBAFLARE_CHANNEL_H
#define GBAFLARE_CHANNEL_H

#include <common/types.h>

#define SWEEP_NUM_MASK BITMASK(3)
#define SWEEP_NUM_SHIFT 0
#define SWEEP_DIR_MASK 1
#define SWEEP_DIR_SHIFT 3
#define SWEEP_TIME_MASK BITMASK(3)
#define SWEEP_TIME_SHIFT 4

#define SOUND_LEN_MASK BITMASK(6)
#define SOUND_LEN_SHIFT 0
#define SOUND_DUTY_MASK BITMASK(2)
#define SOUND_DUTY_SHIFT 6
#define ENV_TIME_MASK BITMASK(3)
#define ENV_TIME_SHIFT 8
#define ENV_DIR_MASK 1
#define ENV_DIR_SHIFT 11
#define ENV_VOL_MASK BITMASK(4)
#define ENV_VOL_SHIFT 12

#define SOUND_FREQ_MASK BITMASK(11)
#define SOUND_FREQ_SHIFT 0
#define LENGTH_ENABLED_MASK 1
#define LENGTH_ENABLED_SHIFT 14
#define SOUND_TRIGGER_MASK 1
#define SOUND_TRIGGER_SHIFT 15

#define WAVE_DIM_MASK 1
#define WAVE_DIM_SHIFT 5
#define WAVE_BANK_MASK 1
#define WAVE_BANK_SHIFT 6
#define WAVE_CH3_ENABLED_MASK 1
#define WAVE_CH3_ENABLED_SHIFT 7

#define WAVE_LEN_MASK BITMASK(8)
#define WAVE_LEN_SHIFT 0
#define WAVE_VOL_MASK BITMASK(2)
#define WAVE_VOL_SHIFT 13
#define WAVE_FORCE_MASK 1
#define WAVE_FORCE_SHIFT 15

#define NOISE_DIV_MASK BITMASK(3)
#define NOISE_DIV_SHIFT 0
#define NOISE_WIDTH_MASK 1
#define NOISE_WIDTH_SHIFT 3
#define NOISE_SHIFT_MASK BITMASK(4)
#define NOISE_SHIFT_SHIFT 4

#define NUM_PSG_CHANNELS 4

extern const int WAVE_DUTY_TABLE[4][8];
extern const addr_t ENVELOPE_ADDR[4];

struct ChannelState {
	bool enabled{};

	s32 freq_timer{};
	u32 wave_pos;

	int current_volume{};
	int period_timer{};

	int length_timer{};
};

struct SweepState {
	bool sweep_enabled{};
	int shadow_freq{};
	int sweep_timer{};

	int calculate_freq();
	void do_sweep_clock();
	void on_trigger();

};

struct NoiseState {
	u16 LFSR{};

};

struct WaveState {
	int current_bank{};
};

int get_psg_value(int ch);
void step_psg(int ch);
void psg_clock_length(int ch);
void psg_clock_envelope(int ch);
void psg_trigger_ch(int ch);
void psg_load_length_timer(int ch, u8 new_value);

extern ChannelState channel_states[NUM_PSG_CHANNELS];
extern SweepState sweep_state;
extern NoiseState noise_state;
extern WaveState wave_state;

#endif
