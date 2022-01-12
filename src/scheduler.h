#ifndef GBAFLARE_SCHEDULER_H
#define GBAFLARE_SCHEDULER_H

#include "types.h"

extern u64 next_event;
extern u64 elapsed;
extern bool scheduler_flag;

void start_event_processing();

void schedule_event(u64 t);
void schedule_after(u64 dt);

void end_event_processing();

extern u64 cpu_cycles;

#endif
