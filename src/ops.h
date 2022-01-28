#ifndef GBAFLARE_OPS_H
#define GBAFLARE_OPS_H

#include "types.h"

#define WRITE_PC(x) \
cpu.pc = (x);\
cpu.flush_pipeline();

#define BRANCH_X_RM \
cpu.set_flag(T_STATE, rm & BIT(0));\
WRITE_PC(rm & 0xFFFFFFFE);

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
V = (rn ^ r) & (operand ^ r) & 0x8000'0000;

#define CV_FLAGS_SUB \
C = !(rn < operand);\
V = (rn ^ operand) & (rn ^ r) & 0x8000'0000;

#define AND_STYLE \
if (rdi == 15) {\
	copy_spsr = true;\
	pc_written = true;\
}\
NZ_FLAGS_RD;

#define TEST_STYLE \
if (rdi == 15) {\
	copy_spsr = true;\
}\
NZ_FLAGS_RD;

#define ADD_STYLE \
AND_STYLE;\
CV_FLAGS_ADD;

#define __WRITE_MULTIPLE(x, target) \
for (int i = 0; i <= (x); i++) {\
	if (register_list & BIT(i)) {\
		cpu.swrite32_noalign(address+rem, (target));\
		address += 4;\
	}\
}\
if (register_list == 0) {\
	cpu.swrite32_noalign(address+rem, cpu.pc + (cpu.in_thumb_state() ? 2 : 4));\
}

#define __READ_MULTIPLE(x, target) \
for (int i = 0; i <= (x); i++) {\
	if (register_list & BIT(i)) {\
		(target) = cpu.sread32_noalign(address+rem);\
		address += 4;\
	}\
}\
if (register_list == 0) {\
	WRITE_PC(cpu.sread32_noalign(address+rem) & (cpu.in_thumb_state() ? 0xFFFF'FFFE : 0xFFFF'FFFC));\
}

#define READ_MULTIPLE(x) \
__READ_MULTIPLE((x), *cpu.get_reg(i));

#define WRITE_MULTIPLE(x) \
__WRITE_MULTIPLE((x), *cpu.get_reg(i));

#define READ_MULTIPLE_FORCE_USER(x) \
__READ_MULTIPLE((x), cpu.registers[0][i]);

#define WRITE_MULTIPLE_FORCE_USER(x) \
__WRITE_MULTIPLE((x), cpu.registers[0][i]);

#define BARREL_SHIFTER(x, k) \
if constexpr (shift_type == 0) {\
	(x) = lsl(rm, imm, k);\
} else if constexpr (shift_type == 1) {\
	(x) = lsr(rm, (imm == 0) ? 32 : imm, k);\
} else if constexpr (shift_type == 2) {\
	(x) = asr(rm, (imm == 0) ? 32 : imm, k);\
} else {\
	if (imm == 0) {\
		(x) = rrx(rm, k);\
	} else {\
		(x) = ror(rm, imm, k);\
	}\
}

#endif
