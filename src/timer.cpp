#include "timer.h"
#include "memory.h"

#include <iostream>

Timer timer;

static const u32 timer_freq[] = {
	1,
	64,
	256,
	1024
};

void Timer::step()
{
	timer_cycles++;

	for (int i = 0; i < NUM_TIMERS; i++) {
		u8 tmcnt = TMCNT_H(i);
		if (!(tmcnt & TIMER_ENABLED)) {
			continue;
		}

		if (tmcnt & TIMER_COUNTUP) {
			continue;
		}

		tcycles[i] += 1;
		const u32 freq = timer_freq[tmcnt & TIMER_PRESCALE];

		if (tcycles[i] >= freq) {
			tcycles[i] -= freq;

			do_timer_increment(i);
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
	const u8 tmcnt = TMCNT_H(i);

	if (tmcnt & TIMER_ENABLEIRQ) {
		request_interrupt(IRQ_TIMER0 * BIT(i));
	}

	if (i < 3) {
		u8 tmcnt2 = TMCNT_H(i+1);
		if ((tmcnt2 & TIMER_ENABLED) && (tmcnt2 & TIMER_COUNTUP)) {
			do_timer_increment(i+1);
		}
	}
}

u8 Timer::on_read(addr_t addr)
{
	const int i = (addr - IO_TM0CNT_L) / 4;

	return values[i] >> (addr % 2 * 8) & BITMASK(8);
}

void Timer::on_write(addr_t addr, u8 value)
{
	const int i = (addr - IO_TM0CNT_L) / 4;

	if (!(TMCNT_H(i) & TIMER_ENABLED) && (value & TIMER_ENABLED)) {
		values[i] = readarr<u16>(io_data, IO_TM0CNT_L - IO_START + i*4);
	}
}
