#include <gba/scheduler.h>

u64 cpu_cycles;
u64 next_event;
u64 elapsed;
bool scheduler_flag = true;

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
	schedule_event(t);
}

void start_event_processing()
{
	scheduler_flag = true;
	elapsed = cpu_cycles;
	cpu_cycles = 0;
}

void end_event_processing()
{
	scheduler_flag = false;
}
