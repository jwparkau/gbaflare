#include "arm.h"
#include "cpu.h"
#include "ops.h"

#include <iostream>
#include <stdexcept>
#include <bit>

constexpr static arm_lut_t init_lut();
constexpr static ArmInstruction *decode_op(u32 op);
constexpr static bool is_alu_op(u32 op);
constexpr static bool is_mul_op(u32 op);
template <u32 link> void arm_branch(u32 op);
void arm_bx(u32 op);
template <u32 is_imm, u32 aluop, u32 set_cond, u32 shift_type, u32 shift_by_reg> void arm_alu(u32 op);
template <u32 mulop, u32 set_cond> void arm_mul(u32 op);
template <u32 r, u32 dir> void arm_psr(u32 op);
template <u32 r> void arm_msr_imm(u32 op);
template <u32 reg_offset, u32 prepost, u32 updown, u32 byteword, u32 writeback, u32 load, u32 shift_type> void arm_sdt(u32 op);
template <u32 prepost, u32 updown, u32 imm_offset, u32 writeback, u32 load, u32 sign, u32 half> void arm_misc_dt(u32 op);
template <u32 prepost, u32 updown, u32 psr, u32 writeback, u32 load> void arm_block_dt(u32 op);
template <u32 byteword> void arm_swp(u32 op);
void arm_swi(u32 op);
void arm_bkpt(u32 op);

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

constexpr static bool is_mul_op(u32 op)
{
	u32 op1 = op >> 4;
	u32 op2 = op & BITMASK(4);

	if (op1 >> 4 == 0 && op2 == 9) {
		u32 mulop = op1 >> 1 & BITMASK(3);

		if (mulop == 0 || mulop == 1 || mulop == 4 || mulop == 5 || mulop == 6 || mulop == 7) {
			return true;
		}
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
	if (op1 == 0x12 && op2 == 1) {
		return arm_bx;
	}

	if (op1 >> 3 == 2 && (op1 & 1) == 0 && op2 == 0) {
		u32 r = op1 >> 2 & 1;
		u32 dir = op1 >> 1 & 1;
#include "PSR_INSTR.gencpp"
	}

	if (op1 >> 3 == 6 && (op1 & BITMASK(2)) == 2) {
		u32 r = op1 >> 2 & 1;
		if (r == 1) {
			return arm_msr_imm<1>;
		} else {
			return arm_msr_imm<0>;
		}
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

	if (op1 >> 3 == 2 && (op1 & BITMASK(2)) == 0 && op2 == 9) {
		u32 byteword = (bool)(op1 >> 2 & 1);
#include "SWP_INSTR.gencpp"
	}

	// multiply
	if (is_mul_op(op)) {
		u32 mulop = op1 >> 1 & BITMASK(4);
		u32 set_cond = (bool)(op1 & 1);
#include "MUL_INSTR.gencpp"
	}

	if (op1 >> 5 == 0 && (op2 & 1) == 1 && (op2 >> 3) == 1 && op2 != 9 && (!(op2 & BIT(2) && (op1 & 1) == 0))) {
		u32 prepost = (bool)(op1 & BIT(4));
		u32 updown = (bool)(op1 & BIT(3));
		u32 imm_offset = (bool)(op1 & BIT(2));
		u32 writeback = (bool)(op1 & BIT(1));
		u32 load = (bool)(op1 & BIT(0));
		u32 sign = (bool)(op2 & BIT(2));
		u32 half = (bool)(op2 & BIT(1));
#include "MISC_DT_INSTR.gencpp"
	}


	// single data transfer
	if (op1 >> 6 == 1) {
		u32 reg_offset = (bool)(op1 & BIT(5));
		u32 prepost = (bool)(op1 & BIT(4));
		u32 updown = (bool)(op1 & BIT(3));
		u32 byteword = (bool)(op1 & BIT(2));
		u32 writeback = (bool)(op1 & BIT(1));
		u32 load = (bool)(op1 & BIT(0));
		u32 shift_type = op2 >> 1 & BITMASK(2);
#include "SDT_INSTR.gencpp"
	}

	if (op1 >> 5 == 4) {
		u32 prepost = (bool)(op1 & BIT(4));
		u32 updown = (bool)(op1 & BIT(3));
		u32 psr = (bool)(op1 & BIT(2));
		u32 writeback = (bool)(op1 & BIT(1));
		u32 load = (bool)(op1 & BIT(0));
#include "BLOCK_DT_INSTR.gencpp"
	}

	if (op1 >> 4 == 15) {
		return arm_swi;
	}

	if (op1 == 18 && op2 == 7) {
		return arm_bkpt;
	}

	return nullptr;
}

constexpr static arm_lut_t init_lut()
{
	arm_lut_t ret;

	for (u32 op = 0; op < ARM_LUT_SIZE; op++) {
		ret[op] = decode_op(op);
	}

	return ret;
}

const arm_lut_t arm_lut = init_lut();

template<u32 link>
void arm_branch(u32 op)
{
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

void arm_bx(u32 op)
{
	u32 rm = *cpu.get_reg(op & BITMASK(4));

	cpu.set_flag(T_STATE, rm & BIT(0));
	cpu.pc = rm & 0xFFFFFFFE;

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

#define NZ_FLAGS_ALU \
do {\
	N = alu_out & BIT(31);\
	Z = (alu_out == 0) ? 1 : 0;\
} while (0);

#define NZ_FLAGS_RD \
do {\
	N = *rd & BIT(31);\
	Z = (*rd == 0) ? 1 : 0;\
} while (0);

#define AND_STYLE \
do {\
	if (rdi == 15) {\
		copy_spsr = true;\
		pc_written = true;\
	} else {\
		NZ_FLAGS_RD;\
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
		u64 result = (u64)rn + operand;
		*rd = result;
		ADD_STYLE;
	} else if constexpr (aluop == OP_ADC) {
		u64 result = (u64)rn + operand + old_carry;
		*rd = result;
		ADD_STYLE;
	} else if constexpr (aluop == OP_SBC) {
		s64 result = (s64)rn - operand - (!old_carry);
		*rd = result;
		AND_STYLE;
		C = result < 0;
		V = (rn ^ operand) & (rn ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_RSC) {
		s64 result = (s64)operand - rn - (!old_carry);
		*rd = result;
		AND_STYLE;
		C = result < 0;
		V = (rn ^ operand) & (operand ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_TST) {
		u32 alu_out = rn & operand;
		NZ_FLAGS_ALU;
	} else if constexpr (aluop == OP_TEQ) {
		u32 alu_out = rn ^ operand;
		NZ_FLAGS_ALU;
	} else if constexpr (aluop == OP_CMP) {
		u32 alu_out = rn - operand;
		NZ_FLAGS_ALU;
		C = !(rn < operand);
		V = (rn ^ operand) & (rn ^ alu_out) & 0x8000'0000;
	} else if constexpr (aluop == OP_CMN) {
		u64 result = (u64)rn + operand;
		u32 alu_out = result;
		NZ_FLAGS_ALU;
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
			cpu.update_mode();
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


template <u32 mulop, u32 set_cond>
void arm_mul(u32 op)
{
	bool N = cpu.CPSR & SIGN_FLAG;
	bool Z = cpu.CPSR & ZERO_FLAG;

	u32 rm = *cpu.get_reg(op & BITMASK(4));
	u32 rs = *cpu.get_reg(op >> 8 & BITMASK(4));
	u32 *rd = cpu.get_reg(op >> 16 & BITMASK(4));
	u32 *rn = cpu.get_reg(op >> 12 & BITMASK(4));

#define NZ_FLAGS_LONG \
do {\
	N = *rd & BIT(31);\
	Z = (*rd == 0 && *rn == 0) ? 1 : 0;\
} while (0);

	if constexpr (mulop == 0) {
		*rd = rm * rs;
		NZ_FLAGS_RD;
	} else if constexpr (mulop == 1) {
		*rd = rm * rs + *rn;
		NZ_FLAGS_RD;
	} else if constexpr (mulop == 4) {
		u64 result = (u64)rm * rs;
		*rd = result >> 32;
		*rn = result;
		NZ_FLAGS_LONG;
	} else if constexpr (mulop == 5) {
		u64 result = (u64)rm * rs;
		u64 result_lo = (result & BITMASK(32)) + *rn;
		bool lo_carry = result_lo > 0xFFFF'FFFF;
		*rn = result_lo;
		*rd = (result >> 32) + *rd + lo_carry;
		NZ_FLAGS_LONG;	
	} else if constexpr (mulop == 6) {
		s64 result = (s64)(s32)rm * (s32)rs;
		*rd = result >> 32;
		*rn = result;
		NZ_FLAGS_LONG;
	} else if constexpr (mulop == 7) {
		s64 result = (s64)(s32)rm * (s32)rs;
		u64 result_lo = (result & BITMASK(32)) + *rn;
		bool lo_carry = result_lo > 0xFFFF'FFFF;
		*rn = result_lo;
		*rd = (result >> 32) + *rd + lo_carry;
		NZ_FLAGS_LONG;
	}

	if constexpr (set_cond) {
		cpu.set_flag(SIGN_FLAG, N);
		cpu.set_flag(ZERO_FLAG, Z);
	}
}

template <u32 r, u32 dir>
void arm_psr(u32 op)
{
	if constexpr (dir == 0) {
		u32 *rd = cpu.get_reg(op >> 12 & BITMASK(4));
		if constexpr (r == 1) {
			*rd = *cpu.get_spsr();
		} else {
			*rd = cpu.CPSR;
		}
	} else {
	u32 rm = *cpu.get_reg(op & BITMASK(4));
	u32 operand = rm;

#define MSR_STYLE \
do {\
	u32 field_mask = op >> 16 & BITMASK(4);\
	if constexpr (r == 0) {\
		if (!cpu.in_privileged_mode()) {\
			field_mask &= ~BITMASK(3);\
		}\
		u32 mask = 0;\
		for (int i = 0; i < 4; i++) {\
			if (field_mask & BIT(i)) {\
				mask |= BITMASK(8) << i * 8;\
			}\
		}\
		cpu.CPSR = (cpu.CPSR & ~mask) | (operand & mask);\
		cpu.update_mode();\
	} else {\
		if (cpu.has_spsr()) {\
			u32 mask = 0;\
			for (int i = 0; i < 4; i++) {\
				if (field_mask & BIT(i)) {\
					mask |= BITMASK(8) << i * 8;\
				}\
			}\
			u32 *spsr = cpu.get_spsr();\
			*spsr = (*spsr & ~mask) | (operand & mask);\
		}\
	}\
} while (0);

	MSR_STYLE;
	}
}

template <u32 r>
void arm_msr_imm(u32 op)
{
	u32 imm = op & BITMASK(8);
	u32 rotate_imm = op >> 8 & BITMASK(4);

	bool carry = cpu.CPSR & CARRY_FLAG;

	u32 operand = ror(imm, rotate_imm * 2, carry);
	cpu.set_flag(CARRY_FLAG, carry);
	MSR_STYLE;
}

template <u32 reg_offset, u32 prepost, u32 updown, u32 byteword, u32 writeback, u32 load, u32 shift_type>
void arm_sdt(u32 op)
{
	u32 address;

	int rdi = op >> 12 & BITMASK(4);
	u32 *rd = cpu.get_reg(rdi);
	u32 *rn = cpu.get_reg(op >> 16 & BITMASK(4));

#define ADDRESS_WRITE \
do {\
	if constexpr (prepost == 1) {\
		if constexpr (updown) {\
			address = *rn + offset;\
		} else {\
			address = *rn - offset;\
		}\
\
		if constexpr (writeback) {\
			*rn = address;\
		}\
	} else {\
		address = *rn;\
		if constexpr (updown) {\
			*rn = *rn + offset;\
		} else {\
			*rn = *rn - offset;\
		}\
	}\
} while (0);

	bool carry = cpu.CPSR & CARRY_FLAG;

	if constexpr (reg_offset == 0) {
		u32 offset = op & BITMASK(12);
		ADDRESS_WRITE;
	} else {
		u32 offset;
		u32 imm = op >> 7 & BITMASK(5);
		u32 rm = *cpu.get_reg(op & BITMASK(4));

		if constexpr (shift_type == 0) {
			offset = lsl(rm, imm, carry);
		} else if constexpr (shift_type == 1) {
			offset = lsr(rm, (imm == 0) ? 32 : imm, carry);
		} else if constexpr (shift_type == 2) {
			offset = asr(rm, (imm == 0) ? 32 : imm, carry);
		} else {
			if (imm == 0) {
				offset = rrx(rm, carry);
			} else {
				offset = ror(rm, imm, carry);
			}
		}
		ADDRESS_WRITE;
	}

	if constexpr (load == 1 && byteword == 0) {
		u32 value;
		u32 addr;
		u32 rem;

		addr = align(address, 4);
		rem = address & BITMASK(2);
		
		value = ror(cpu.cpu_read32(addr), rem * 8, carry);

		if (rdi == 15) {
			cpu.pc = align(value, 4);
		} else {
			*rd = value;
		}
	} else if constexpr (load == 1 && byteword == 1) {
		*rd = cpu.cpu_read8(address);
	} else if constexpr (load == 0 && byteword == 0) {
		cpu.cpu_write32(align(address, 4), *rd);
	} else if constexpr (load == 0 && byteword == 1) {
		cpu.cpu_write8(address, *rd & BITMASK(8));
	}

	cpu.set_flag(CARRY_FLAG, carry);
}

template <u32 byteword>
void arm_swp(u32 op)
{
	u32 rn = *cpu.get_reg(op >> 16 & BITMASK(4));
	u32 rm = *cpu.get_reg(op & BITMASK(4));
	u32 *rd = cpu.get_reg(op >> 12 & BITMASK(4));

	bool carry = cpu.CPSR & CARRY_FLAG;


	if constexpr (byteword == 0) {
		u32 addr = align(rn, 4);
		u32 rem = rn & BITMASK(2);
		u32 temp = ror(cpu.cpu_read32(addr), rem * 8, carry);

		cpu.cpu_write32(addr, rm);
		*rd = temp;
	} else {
		u32 temp = cpu.cpu_read8(rn);
		cpu.cpu_write8(rn, rm & BITMASK(8));
		*rd = temp;
	}

	cpu.set_flag(CARRY_FLAG, carry);
}

template <u32 prepost, u32 updown, u32 imm_offset, u32 writeback, u32 load, u32 sign, u32 half>
void arm_misc_dt(u32 op)
{
	u32 address;

	u32 *rn = cpu.get_reg(op >> 16 & BITMASK(4));
	u32 *rd = cpu.get_reg(op >> 12 & BITMASK(4));

	if constexpr (imm_offset == 1) {
		u32 offset = ((op >> 8 & BITMASK(4)) << 4) | (op & BITMASK(4));
		ADDRESS_WRITE;
	} else {
		u32 offset = *cpu.get_reg(op & BITMASK(4));
		ADDRESS_WRITE;
	}

	if constexpr (load == 1 && half == 0) {
		*rd = (s8)cpu.cpu_read8(address);	
	} else if constexpr (load == 1 && half == 1) {
		if constexpr (sign == 1) {
			*rd = (s16)cpu.cpu_read16(align(address, 2));
		} else {
			*rd = cpu.cpu_read16(align(address, 2));
		}
	} else if (load == 0 && half == 1) {
		cpu.cpu_write16(align(address, 2), *rd & BITMASK(16));
	}
}

template <u32 prepost, u32 updown, u32 psr, u32 writeback, u32 load>
void arm_block_dt(u32 op)
{
	u32 start_address;
	u32 end_address;

	u32 *rn = cpu.get_reg(op >> 16 & BITMASK(4));
	u32 register_list = op & BITMASK(16);
	u32 k = std::popcount(register_list) * 4;

	if constexpr (prepost == 0 && updown == 1) {
		start_address = *rn;
		end_address = *rn + k - 4;
		if constexpr (writeback) {
			*rn = *rn + k;
		}
	} else if constexpr (prepost == 1 && updown == 1) {
		start_address = *rn + 4;
		end_address = *rn + k;
		if constexpr (writeback) {
			*rn = *rn + k;
		}
	} else if constexpr (prepost == 0 && updown == 0) {
		start_address = *rn - k + 4;
		end_address = *rn;
		if constexpr (writeback) {
			*rn = *rn - k;
		}
	} else {
		start_address = *rn - k;
		end_address = *rn - 4;
		if constexpr (writeback) {
			*rn = *rn - k;
		}
	}

	if constexpr (load == 1 && psr == 0) {
		u32 address = start_address;

		for (int i = 0; i <= 14; i++) {
			if (register_list & BIT(i)) {
				*cpu.get_reg(i) = cpu.cpu_read32(align(address, 4));
				address += 4;
			}
		}

		if (register_list & BIT(15)) {
			u32 value = cpu.cpu_read32(align(address, 4));
			cpu.pc = value & 0xFFFF'FFFC;
			cpu.flush_pipeline();
			address += 4;
		}
	} else if (load == 1 && writeback == 0 && psr == 1 && !(op & BIT(15))) {
		u32 address = start_address;
		for (int i = 0; i <= 14; i++) {
			if (register_list & BIT(i)) {
				cpu.registers[0][i] = cpu.cpu_read32(align(address, 4));
				address += 4;
			}
		}
	} else if (load == 1 && psr == 1 && (op & BIT(15))) {
		u32 address = start_address;
		for (int i = 0; i <= 14; i++) {
			if (register_list & BIT(i)) {
				*cpu.get_reg(i) = cpu.cpu_read32(align(address, 4));
				address += 4;
			}
		}

		cpu.CPSR = *cpu.get_spsr();
		cpu.update_mode();

		u32 value = cpu.cpu_read32(align(address, 4));
		if (cpu.CPSR & T_STATE) {
			cpu.pc = value & 0xFFFF'FFFE;
		} else {
			cpu.pc = value & 0xFFFF'FFFC;
		}

		cpu.flush_pipeline();
	} else if constexpr (load == 0 && psr == 0) {
		u32 address = start_address;
		for (int i = 0; i <= 15; i++) {
			if (register_list & BIT(i)) {
				cpu.cpu_write32(align(address, 4), *cpu.get_reg(i));
				address += 4;
			}
		}
	} else if constexpr (load == 0 && writeback == 0 && psr == 1) {
		u32 address = start_address;
		for (int i = 0; i <= 14; i++) {
			if (register_list & BIT(i)) {
				cpu.cpu_write32(align(address, 4), cpu.registers[0][i]);
				address += 4;
			}
		}

		if (register_list & BIT(15)) {
			cpu.cpu_write32(align(address, 4), cpu.pc);
			address += 4;
		}
	}
}

void arm_swi(u32 op)
{
	cpu.registers[SUPERVISOR][14] = cpu.pc - 4;
	cpu.SPSR[SUPERVISOR] = cpu.CPSR;

	cpu.CPSR = (cpu.CPSR & ~BITMASK(5)) | 0x13;
	cpu.update_mode();
	cpu.set_flag(T_STATE, 0);
	cpu.set_flag(IRQ_DISABLE, 1);

	cpu.pc = VECTOR_SWI;
}

void arm_bkpt(u32 op)
{
	cpu.registers[ABORT][14] = cpu.pc - 4;
	cpu.SPSR[ABORT] = cpu.CPSR;

	cpu.CPSR = (cpu.CPSR & ~BITMASK(5)) | 0x17;
	cpu.update_mode();
	cpu.set_flag(T_STATE, 0);
	cpu.set_flag(IRQ_DISABLE, 1);

	cpu.pc = VECTOR_PREFETCH_ABORT;
}
