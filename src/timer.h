#ifndef GBAFLARE_TIMER_H
#define GBAFLARE_TIMER_H

#include "types.h"

#define NUM_TIMERS 4

enum tmcnt_flags {
	TIMER_PRESCALE	= 0x3,
	TIMER_COUNTUP	= 0x4,
	TIMER_ENABLEIRQ	= 0x40,
	TIMER_ENABLED	= 0x80
};


#define TMCNT_L(x) io_data[IO_TM0CNT_L - IO_START + 4*(x)]
#define TMCNT_H(x) io_data[IO_TM0CNT_H - IO_START + 4*(x)]

struct Timer {
	u64 timer_cycles{};
	u64 last_timer_update{};

	u32 tcycles[NUM_TIMERS]{};
	u32 values[NUM_TIMERS]{};
	u16 reload[NUM_TIMERS]{};

	void step();
	void simulate_elapsed(u64 dt);
	void do_timer_increment(int i);
	void on_timer_overflow(int i);

	u8 on_read(addr_t addr);
	void on_write(addr_t addr, u8 value);
};

extern Timer timer;

#endif
