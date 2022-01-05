#include "arm.h"
#include "cpu.h"
#include "ops.h"

#include <iostream>

constexpr static arm_lut_t init_lut();
constexpr static ArmInstruction *decode_op(u32 op);
constexpr static bool is_alu_op(u32 op);
template <u32 link> void arm_branch(u32 op);
template <u32 is_imm, u32 aluop, u32 set_cond, u32 shift_type, u32 shift_by_reg> void arm_alu(u32 op);

constexpr static bool is_alu_op(u32 op)
{
	u32 op1 = op >> 4;
	u32 op2 = op & BITMASK(4);

	if (op1 >> 6 == 0) {
		u32 is_imm = (bool)(op1 & BIT(5));
		u32 aluop = (op1 >> 1 & BITMASK(4));
		u32 set_cond = (bool)(op1 & 1);

		if (8 <= aluop && aluop <= 0xB && set_cond != 1) {
			return false;
		}

		if (is_imm == 0) {
			u32 shift_by_reg = (bool)(op2 & 1);
			if (shift_by_reg && (op2 >> 3 & 1) != 0) {
				return false;
			}
		}

		return true;
	}
	return false;
}


constexpr static ArmInstruction *decode_op(u32 op)
{
	u32 op1 = op >> 4;
	u32 op2 = op & BITMASK(4);

	// branch instructions
	if (op1 >> 5 == 5) {
		u32 link = op1 >> 4 & 1;
#include "BRANCH_INSTR.gencpp"
	}

	// misc
	if (op1 >> 3 == 2 && (op1 & 1) == 0) {
		return nullptr;
	}

	// alu
	if (is_alu_op(op)) {
		u32 is_imm = (bool)(op1 >> 5 & 1);
		u32 aluop = op1 >> 1 & BITMASK(4);
		u32 set_cond = (bool)(op1 & 1);
		u32 shift_type = op2 >> 1 & BITMASK(2);
		u32 shift_by_reg = (bool)(op2 & 1);
#include "ALU_INSTR.gencpp"
	}
	return nullptr;
}

constexpr static arm_lut_t init_lut()
{
	arm_lut_t ret;

	for (u32 op = 0; op < 4096; op++) {
		ret[op] = decode_op(op);
	}

	return ret;
}

const arm_lut_t arm_lut = init_lut();

template<u32 link>
void arm_branch(u32 op)
{
	// TODO: check cond for 1111

	u32 offset = op & BITMASK(24);
	offset <<= 8;
	s32 nn = offset;
	nn >>= 8;

	if constexpr (link) {
		*cpu.lr = cpu.pc - 4;
	}

	cpu.pc = cpu.pc + nn * 4;
	cpu.flush_pipeline();
}

template <u32 is_imm, u32 aluop, u32 set_cond, u32 shift_type, u32 shift_by_reg>
void arm_alu(u32 op)
{
	u32 operand;
	bool N = cpu.CPSR & SIGN_FLAG;
	bool Z = cpu.CPSR & ZERO_FLAG;
	bool C = cpu.CPSR & CARRY_FLAG;
	bool old_carry = C;
	bool V = cpu.CPSR & OVERFLOW_FLAG;

	if constexpr (is_imm) {
		u32 imm = op & BITMASK(8);
		u32 rotate_imm = op >> 8 & BITMASK(4);
		operand = ror(imm, rotate_imm * 2, C);
	} else {
		u32 rm = *cpu.get_reg(op & BITMASK(4));
		if constexpr (shift_by_reg) {
			u32 rs = *cpu.get_reg(op >> 8 & BITMASK(4)) & BITMASK(8);
			if constexpr (shift_type == 0) {
				operand = lsl(rm, rs, C);
			} else if constexpr (shift_type == 1) {
				operand = lsr(rm, rs, C);
			} else if constexpr (shift_type == 2) {
				operand = asr(rm, rs, C);
			} else {
				operand = ror(rm, rs, C);
			}
		} else {
			u32 imm = op >> 7 & BITMASK(5);
			if constexpr (shift_type == 0) {
				operand = lsl(rm, imm, C);
			} else if constexpr (shift_type == 1) {
				operand = lsr(rm, (imm == 0) ? 32 : imm, C);
			} else if constexpr (shift_type == 2) {
				operand = asr(rm, (imm == 0) ? 32 : imm, C);
			} else {
				if (imm == 0) {
					operand = rrx(rm, C);
				} else {
					operand = ror(rm, imm, C);
				}
			}
		}
	}

	u32 rn = *cpu.get_reg(op >> 16 & BITMASK(4));
	u32 rdi = op >> 12 & BITMASK(4);
	u32 *rd = cpu.get_reg(rdi);
	u32 spsr = *cpu.get_spsr();

	bool copy_spsr = false;
	bool pc_written = false;

#define NZ_FLAGS \
do {\
	N = alu_out & BIT(31);\
	Z = (alu_out == 0) ? 1 : 0;\
} while (0);

#define AND_STYLE \
do {\
	if (rdi == 15) {\
		copy_spsr = true;\
		pc_written = true;\
	} else {\
		N = *rd & BIT(31);\
		Z = (*rd == 0) ? 1 : 0;\
	}\
} while (0);

#define ADD_STYLE \
AND_STYLE \
do {\
	C = result > 0xFFFF'FFFF;\
	V = (rn ^ *rd) & (operand & *rd) & 0x8000'0000;\
} while (0);

	if constexpr (aluop == OP_AND) {
		*rd = rn & operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_EOR) {
		*rd = rn ^ operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_SUB) {
		*rd = rn - operand;
		AND_STYLE;
		C = !(rn < operand);
		V = (rn ^ operand) & (rn ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_RSB) {
		*rd = operand - rn;
		AND_STYLE;
		C = !(operand < rn);
		V = (rn ^ operand) & (operand ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_ADD) {
		u64 result = rn + operand;
		*rd = result;
		ADD_STYLE;
	} else if constexpr (aluop == OP_ADC) {
		u64 result = rn + operand + old_carry;
		*rd = result;
		ADD_STYLE;
	} else if constexpr (aluop == OP_SBC) {
		s64 result = rn - operand - (!old_carry);
		*rd = result;
		AND_STYLE;
		C = result < 0;
		V = (rn ^ operand) & (rn ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_RSC) {
		s64 result = operand - rn - (!old_carry);
		*rd = result;
		AND_STYLE;
		C = result < 0;
		V = (rn ^ operand) & (operand ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_TST) {
		u32 alu_out = rn & operand;
		NZ_FLAGS;
	} else if constexpr (aluop == OP_TEQ) {
		u32 alu_out = rn ^ operand;
		NZ_FLAGS;
	} else if constexpr (aluop == OP_CMP) {
		u32 alu_out = rn - operand;
		NZ_FLAGS;
		C = !(rn < operand);
		V = (rn ^ operand) & (rn ^ alu_out) & 0x8000'0000;
	} else if constexpr (aluop == OP_CMN) {
		u64 result = rn + operand;
		u32 alu_out = result;
		NZ_FLAGS;
		C = result > 0xFFFF'FFFF;
		V = (rn ^ alu_out) & (operand & alu_out) & 0x8000'0000;
	} else if constexpr (aluop == OP_ORR) {
		*rd = rn | operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_MOV) {
		*rd = operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_BIC) {
		*rd = rn & (~operand);
		AND_STYLE;
	} else if constexpr (aluop == OP_MVN) {
		*rd = ~operand;
		AND_STYLE;
	}

	if constexpr (set_cond) {
		if (copy_spsr) {
			cpu.CPSR = spsr;
		} else {
			cpu.set_flag(SIGN_FLAG, N);
			cpu.set_flag(ZERO_FLAG, Z);
			cpu.set_flag(CARRY_FLAG, C);
			cpu.set_flag(OVERFLOW_FLAG, V);
		}
	}

	if (pc_written) {
		if (*rd % 4 != 0) {
			fprintf(stderr, "unaligned write to pc %08X\n", *rd);
			*rd = *rd & (~BITMASK(2));
		}
		cpu.flush_pipeline();
	}
}
