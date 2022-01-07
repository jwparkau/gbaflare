#ifndef GBAFLARE_OPS_H
#define GBAFLARE_OPS_H

#include "types.h"

u32 ror(u32 x, u32 n, bool &carry);
u32 asr(u32 x, u32 n, bool &carry);
u32 lsr(u32 x, u32 n, bool &carry);
u32 lsl(u32 x, u32 n, bool &carry);
u32 rrx(u32 x, bool &carry);

u32 align(u32 addr, u32 x);

#define GET_CPU_FLAGS \
bool N = cpu.CPSR & SIGN_FLAG;\
bool Z = cpu.CPSR & ZERO_FLAG;\
bool C = cpu.CPSR & CARRY_FLAG;\
bool V = cpu.CPSR & OVERFLOW_FLAG;

#define WRITE_CPU_FLAGS \
cpu.set_flag(SIGN_FLAG, N);\
cpu.set_flag(ZERO_FLAG, Z);\
cpu.set_flag(CARRY_FLAG, C);\
cpu.set_flag(OVERFLOW_FLAG, V);

#define NZ_FLAGS_RD \
N = r & BIT(31);\
Z = (r == 0) ? 1 : 0;

#define CV_FLAGS_ADD \
C = result > 0xFFFF'FFFF;\
V = (rn ^ r) & (operand & r) & 0x8000'0000;

#define CV_FLAGS_SUB \
C = !(rn < operand);\
V = (rn ^ operand) & (rn ^ r) & 0x8000'0000;

#define AND_STYLE \
if (rdi == 15) {\
	copy_spsr = true;\
	pc_written = true;\
}\
NZ_FLAGS_RD;

#define ADD_STYLE \
AND_STYLE;\
CV_FLAGS_ADD;

#endif
