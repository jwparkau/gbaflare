#ifndef GBAFLARE_OPS_H
#define GBAFLARE_OPS_H

#include "types.h"

u32 ror(u32 x, u32 n, bool &carry);
u32 asr(u32 x, u32 n, bool &carry);
u32 lsr(u32 x, u32 n, bool &carry);
u32 lsl(u32 x, u32 n, bool &carry);
u32 rrx(u32 x, bool &carry);

u32 align(u32 addr, u32 x);

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

#define ADD_STYLE \
AND_STYLE;\
CV_FLAGS_ADD;

#define __WRITE_MULTIPLE(x, target) \
for (int i = 0; i <= (x); i++) {\
	if (register_list & BIT(i)) {\
		cpu.cpu_write32(address, (target));\
		address += 4;\
	}\
}

#define __READ_MULTIPLE(x, target) \
for (int i = 0; i <= (x); i++) {\
	if (register_list & BIT(i)) {\
		(target) = cpu.cpu_read32(address);\
		address += 4;\
	}\
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


#define EXCEPTION_PROLOGUE(mode, flag) \
cpu.registers[mode][14] = cpu.pc - 4;\
cpu.SPSR[mode] = cpu.CPSR;\
cpu.CPSR = (cpu.CPSR & ~BITMASK(5)) | (flag);\
cpu.update_mode();\
cpu.set_flag(T_STATE, 0);\
cpu.set_flag(IRQ_DISABLE, 1);

#endif
