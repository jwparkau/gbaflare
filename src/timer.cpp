#include "timer.h"
#include "memory.h"
#include "scheduler.h"

#include <iostream>

Timer timer;

static const int timer_freq[] = {
	1,
	64,
	256,
	1024
};

void Timer::step()
{
	simulate_elapsed(elapsed - last_timer_update);

	last_timer_update = 0;
}

void Timer::simulate_elapsed(u64 dt)
{
	for (int i = 0; i < NUM_TIMERS; i++) {
		u8 tmcnt = TMCNT_H(i);
		if (!(tmcnt & TIMER_ENABLED)) {
			continue;
		}

		if (tmcnt & TIMER_COUNTUP) {
			continue;
		}

		tcycles[i] += dt;
		u32 freq = timer_freq[tmcnt & TIMER_PRESCALE];

		bool value_changed = false;

		while (tcycles[i] >= freq) {
			tcycles[i] -= freq;

			do_timer_increment(i);
			value_changed = true;
		}

		if (value_changed) {
			schedule_after((0x10000 - values[i]) * freq);
		}
	}
}

void Timer::do_timer_increment(int i)
{
	values[i] += 1;

	if (values[i] > 0xFFFF) {
		values[i] &= 0xFFFF;

		/* timer i overflows here */
		on_timer_overflow(i);
	}

}

void Timer::on_timer_overflow(int i)
{
	u8 tmcnt = TMCNT_H(i);

	if (tmcnt & TIMER_ENABLEIRQ) {
		request_interrupt(IRQ_TIMER0 * BIT(i));
	}

	if (i < 3) {
		u8 tmcnt2 = TMCNT_H(i+1);
		if ((tmcnt2 & TIMER_ENABLED) && (tmcnt2 & TIMER_COUNTUP)) {
			do_timer_increment(i+1);
		}
	}

	values[i] = readarr<u16>(io_data, IO_TM0CNT_L - IO_START + i*4);
}

u8 Timer::on_read(addr_t addr)
{
	int i = (addr - IO_TM0CNT_L) / 4;

	simulate_elapsed(cpu_cycles - last_timer_update);
	last_timer_update = cpu_cycles;

	return values[i] >> (addr % 2 * 8) & BITMASK(8);
}

void Timer::on_write(addr_t addr, u8 old_value, u8 new_value)
{
	int i = (addr - IO_TM0CNT_L) / 4;

	simulate_elapsed(cpu_cycles - last_timer_update);
	last_timer_update = cpu_cycles;

	if (!(old_value & TIMER_ENABLED) && (new_value & TIMER_ENABLED)) {
		values[i] = readarr<u16>(io_data, IO_TM0CNT_L - IO_START + i*4);

		if (!(new_value & TIMER_COUNTUP)) {
			u32 freq = timer_freq[new_value & TIMER_PRESCALE];
			schedule_after((0x10000 - values[i]) * freq);
		}
	}
}
