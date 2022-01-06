#ifndef GBAFLARE_OPS_H
#define GBAFLARE_OPS_H

#include "types.h"

u32 ror(u32 x, u32 n, bool &carry);
u32 asr(u32 x, u32 n, bool &carry);
u32 lsr(u32 x, u32 n, bool &carry);
u32 lsl(u32 x, u32 n, bool &carry);
u32 rrx(u32 x, bool &carry);

u32 align(u32 addr, u32 x);

#endif
