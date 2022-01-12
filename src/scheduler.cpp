#include "scheduler.h"
#include "timer.h"

u64 cpu_cycles;
u64 next_event;
u64 elapsed;
bool scheduler_flag = true;

static u64 last_event;

void schedule_event(u64 t)
{
	if (scheduler_flag || t < next_event) {
		next_event = t;
		scheduler_flag = false;
	}
}

void schedule_after(u64 dt)
{
	u64 t = cpu_cycles + dt;
	if (t < cpu_cycles) {
		next_event -= cpu_cycles;
		cpu_cycles = 0;
		t = dt;
		last_event = 0;
		timer.last_timer_update = 0;
	}
	schedule_event(t);
}

void start_event_processing()
{
	scheduler_flag = true;
	elapsed = cpu_cycles - last_event;
	last_event = cpu_cycles;
}

void end_event_processing()
{
	scheduler_flag = false;
}
