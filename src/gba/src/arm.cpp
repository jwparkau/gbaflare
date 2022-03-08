#include <gba/arm.h>
#include <gba/cpu.h>
#include <gba/ops.h>
#include <gba/memory.h>

#include <iostream>
#include <stdexcept>
#include <bit>

template <u32 link> void arm_branch(u32 op);
void arm_bx(u32 op);
template <u32 is_imm, u32 aluop, u32 set_cond, u32 shift_type, u32 shift_by_reg> void arm_alu(u32 op);
template <u32 mulop, u32 set_cond> void arm_mul(u32 op);
template <u32 psr, u32 dir> void arm_psr(u32 op);
template <u32 psr> void arm_msr_imm(u32 op);
template <u32 reg_offset, u32 prepost, u32 updown, u32 byteword, u32 writeback, u32 load, u32 shift_type> void arm_sdt(u32 op);
template <u32 prepost, u32 updown, u32 imm_offset, u32 writeback, u32 load, u32 sign, u32 half> void arm_misc_dt(u32 op);
template <u32 prepost, u32 updown, u32 psr, u32 writeback, u32 load> void arm_block_dt(u32 op);
template <u32 byteword> void arm_swp(u32 op);
void arm_swi(u32 op);
void arm_bkpt(u32 op);

#include <gba/cpu/arm_lut.cpp>

template<u32 link>
void arm_branch(u32 op)
{
	u32 imm = op & BITMASK(24);
	s32 nn = (s32)(imm << 8) >> 8;

	if constexpr (link) {
		u32 *lr = cpu.get_lr();
		*lr = cpu.pc - 4;
	}

	WRITE_PC(cpu.pc + nn * 4);
	cpu.sfetch();
}

void arm_bx(u32 op)
{
	u32 rm = *cpu.get_reg(op & BITMASK(4));

	BRANCH_X_RM;
	cpu.sfetch();
}

template <u32 is_imm, u32 aluop, u32 set_cond, u32 shift_type, u32 shift_by_reg>
void arm_alu(u32 op)
{
	GET_CPU_FLAGS;

	u32 operand;
	bool old_carry = C;

	if constexpr (is_imm) {
		u32 imm = op & BITMASK(8);
		u32 rotate_imm = op >> 8 & BITMASK(4);
		operand = ror(imm, rotate_imm * 2, C);
	} else {
		u32 rmi = op & BITMASK(4);
		u32 rm = *cpu.get_reg(rmi);

		if constexpr (shift_by_reg) {
			cpu.icycle();
			if (rmi == 15) {
				rm += 4;
			}
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
			BARREL_SHIFTER(operand, C);
		}
	}

	u32 rni = op >> 16 & BITMASK(4);
	u32 rn = *cpu.get_reg(rni);

	if constexpr (is_imm == 0 && shift_by_reg == 1) {
		if (rni == 15) {
			rn += 4;
		}
	}

	u32 rdi = op >> 12 & BITMASK(4);
	u32 *rd = cpu.get_reg(rdi);

	bool copy_spsr = false;
	bool pc_written = false;

	if constexpr (aluop == OP_AND) {
		u32 r = *rd = rn & operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_EOR) {
		u32 r = *rd = rn ^ operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_SUB) {
		u32 r = *rd = rn - operand;
		AND_STYLE;
		CV_FLAGS_SUB;
	} else if constexpr (aluop == OP_RSB) {
		u32 r = *rd = operand - rn;
		AND_STYLE;
		C = !(operand < rn);
		V = (rn ^ operand) & (operand ^ *rd) & 0x8000'0000;
	} else if constexpr (aluop == OP_ADD) {
		u64 result = (u64)rn + operand;
		u32 r = *rd = result;
		ADD_STYLE;
	} else if constexpr (aluop == OP_ADC) {
		u64 result = (u64)rn + operand + old_carry;
		u32 r = *rd = result;
		ADD_STYLE;
	} else if constexpr (aluop == OP_SBC) {
		s64 result = (s64)rn - operand - (!old_carry);
		u32 r = *rd = result;
		AND_STYLE;
		C = !(result < 0);
		V = (rn ^ operand) & (rn ^ r) & 0x8000'0000;
	} else if constexpr (aluop == OP_RSC) {
		s64 result = (s64)operand - rn - (!old_carry);
		u32 r = *rd = result;
		AND_STYLE;
		C = !(result < 0);
		V = (rn ^ operand) & (operand ^ r) & 0x8000'0000;
	} else if constexpr (aluop == OP_TST) {
		u32 r = rn & operand;
		TEST_STYLE;
	} else if constexpr (aluop == OP_TEQ) {
		u32 r = rn ^ operand;
		TEST_STYLE;
	} else if constexpr (aluop == OP_CMP) {
		u32 r = rn - operand;
		TEST_STYLE;
		CV_FLAGS_SUB;
	} else if constexpr (aluop == OP_CMN) {
		u64 result = (u64)rn + operand;
		u32 r = result;
		TEST_STYLE;
		CV_FLAGS_ADD;
	} else if constexpr (aluop == OP_ORR) {
		u32 r = *rd = rn | operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_MOV) {
		u32 r = *rd = operand;
		AND_STYLE;
	} else if constexpr (aluop == OP_BIC) {
		u32 r = *rd = rn & (~operand);
		AND_STYLE;
	} else if constexpr (aluop == OP_MVN) {
		u32 r = *rd = ~operand;
		AND_STYLE;
	}

	if constexpr (set_cond) {
		if (copy_spsr) {
			cpu.CPSR = *cpu.get_spsr();
			cpu.update_mode();
		} else {
			WRITE_CPU_FLAGS;
		}
	}

	if (pc_written) {
		*rd = align(*rd, cpu.in_thumb_state() ? 2 : 4);
		cpu.flush_pipeline();
	}

	if constexpr (!is_imm && shift_by_reg) {
		if (!prefetch_enabled && !pc_written) {
			cpu.nfetch();
			return;
		}
	}

	cpu.sfetch();
}


template <u32 mulop, u32 set_cond>
void arm_mul(u32 op)
{
	GET_CPU_FLAGS;

	u32 rm = *cpu.get_reg(op & BITMASK(4));
	u32 rs = *cpu.get_reg(op >> 8 & BITMASK(4));
	u32 *rd = cpu.get_reg(op >> 16 & BITMASK(4));
	u32 *rn = cpu.get_reg(op >> 12 & BITMASK(4));

#define NZ_FLAGS_LONG N = *rd & BIT(31); Z = (*rd == 0 && *rn == 0) ? 1 : 0;

#define MULTIPLY_LONG_CARRY \
u64 result_lo = (result & BITMASK(32)) + *rn;\
bool lo_carry = result_lo > 0xFFFF'FFFF;\
*rn = result_lo;\
*rd = (result >> 32) + *rd + lo_carry;\
NZ_FLAGS_LONG;

#define MULTIPLY_LONG \
*rd = result >> 32;\
*rn = result;\
NZ_FLAGS_LONG;

	if constexpr (mulop == 0) {
		u32 r = *rd = rm * rs;
		NZ_FLAGS_RD;
		MUL_ONES_ZEROS;
		cpu.icycle(m);
	} else if constexpr (mulop == 1) {
		u32 r = *rd = rm * rs + *rn;
		NZ_FLAGS_RD;
		MUL_ONES_ZEROS;
		cpu.icycle(m+1);
	} else if constexpr (mulop == 4) {
		u64 result = (u64)rm * rs;
		MULTIPLY_LONG;
		MUL_ZEROS;
		cpu.icycle(m+1);
	} else if constexpr (mulop == 5) {
		u64 result = (u64)rm * rs;
		MULTIPLY_LONG_CARRY;
		MUL_ZEROS;
		cpu.icycle(m+2);
	} else if constexpr (mulop == 6) {
		s64 result = (s64)(s32)rm * (s32)rs;
		MULTIPLY_LONG;
		MUL_ONES_ZEROS;
		cpu.icycle(m+1);
	} else if constexpr (mulop == 7) {
		s64 result = (s64)(s32)rm * (s32)rs;
		MULTIPLY_LONG_CARRY;
		MUL_ONES_ZEROS;
		cpu.icycle(m+2);
	}

	if constexpr (set_cond) {
		WRITE_CPU_FLAGS;
	}

	if (!prefetch_enabled) {
		cpu.nfetch();
	} else {
		cpu.sfetch();
	}
}

template <u32 psr, u32 dir>
void arm_psr(u32 op)
{
	if constexpr (dir == 0) {
		u32 *rd = cpu.get_reg(op >> 12 & BITMASK(4));
		if constexpr (psr == 1) {
			*rd = *cpu.get_spsr();
		} else {
			*rd = cpu.CPSR;
		}
	} else {
		u32 operand = *cpu.get_reg(op & BITMASK(4));

#define CREATE_MASK \
u32 mask = 0;\
for (int i = 0; i < 4; i++) {\
	if (field_mask & BIT(i)) {\
		mask |= BITMASK(8) << (i * 8);\
	}\
}

#define MSR_STYLE \
u32 field_mask = op >> 16 & BITMASK(4);\
if constexpr (psr == 0) {\
	if (!cpu.in_privileged_mode()) {\
		field_mask &= ~BITMASK(3);\
	}\
	CREATE_MASK;\
	cpu.CPSR = (cpu.CPSR & ~mask) | (operand & mask);\
	cpu.update_mode();\
} else {\
	if (cpu.has_spsr()) {\
		CREATE_MASK;\
		u32 *spsr = cpu.get_spsr();\
		*spsr = (*spsr & ~mask) | (operand & mask);\
	}\
}

		MSR_STYLE;
	}
	cpu.sfetch();
}

template <u32 psr>
void arm_msr_imm(u32 op)
{
	u32 imm = op & BITMASK(8);
	u32 rotate_imm = op >> 8 & BITMASK(4);

	bool carry = cpu.CPSR & CARRY_FLAG;

	u32 operand = ror(imm, rotate_imm * 2, carry);
	cpu.set_flag(CARRY_FLAG, carry);

	MSR_STYLE;
	cpu.sfetch();
}

template <u32 reg_offset, u32 prepost, u32 updown, u32 byteword, u32 writeback, u32 load, u32 shift_type>
void arm_sdt(u32 op)
{
	u32 address;

	int rdi = op >> 12 & BITMASK(4);
	u32 *rd = cpu.get_reg(rdi);
	int rni = op >> 16 & BITMASK(4);
	u32 *rn = cpu.get_reg(rni);
	u32 old_rn = *rn;

#define ADDRESS_WRITE \
if constexpr (prepost == 1) {\
	if constexpr (updown) {\
		address = *rn + offset;\
	} else {\
		address = *rn - offset;\
	}\
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
}

	bool carry = cpu.CPSR & CARRY_FLAG;

	if constexpr (reg_offset == 0) {
		u32 offset = op & BITMASK(12);
		ADDRESS_WRITE;
	} else {
		u32 offset;
		u32 imm = op >> 7 & BITMASK(5);
		u32 rm = *cpu.get_reg(op & BITMASK(4));

		BARREL_SHIFTER(offset, carry);
		ADDRESS_WRITE;
	}

	if constexpr (load == 1 && byteword == 0) {
		u32 value;
		u32 rem;

		rem = address & BITMASK(2);
		value = ror(cpu.nread32_noalign(address), rem * 8, carry);

		if (rdi == 15) {
			WRITE_PC(align(value, 4));
		} else {
			*rd = value;
		}
	} else if constexpr (load == 1 && byteword == 1) {
		*rd = cpu.nread8(address);
	} else if constexpr (load == 0 && byteword == 0) {
		u32 value = (rdi == rni) ? old_rn : *rd;
		cpu.nwrite32_noalign(address, value + (rdi == 15 ? 4 : 0));
	} else if constexpr (load == 0 && byteword == 1) {
		cpu.nwrite8(address, (*rd + (rdi == 15 ? 4 : 0)) & BITMASK(8));
	}

	if constexpr (load == 1) {
		cpu.icycle();
		if (!prefetch_enabled) {
			cpu.nfetch();
		} else {
			cpu.sfetch();
		}
	} else {
		cpu.nfetch();
	}
}

template <u32 byteword>
void arm_swp(u32 op)
{
	u32 rn = *cpu.get_reg(op >> 16 & BITMASK(4));
	u32 rm = *cpu.get_reg(op & BITMASK(4));
	u32 *rd = cpu.get_reg(op >> 12 & BITMASK(4));

	bool carry = cpu.CPSR & CARRY_FLAG;

	u32 address = rn;

	cpu.icycle();

	if constexpr (byteword == 0) {
		u32 rem = address & BITMASK(2);
		u32 temp = ror(cpu.nread32_noalign(address), rem * 8, carry);

		cpu.nwrite32_noalign(address, rm);
		*rd = temp;
	} else {
		u32 temp = cpu.nread8(rn);
		cpu.nwrite8(rn, rm & BITMASK(8));
		*rd = temp;
	}

	if (!prefetch_enabled) {
		cpu.nfetch();
	} else {
		cpu.sfetch();
	}
}

template <u32 prepost, u32 updown, u32 imm_offset, u32 writeback, u32 load, u32 sign, u32 half>
void arm_misc_dt(u32 op)
{
	u32 address;

	u32 rni = op >> 16 & BITMASK(4);
	u32 *rn = cpu.get_reg(rni);
	u32 old_rn = *rn;
	u32 rdi = op >> 12 & BITMASK(4);
	u32 *rd = cpu.get_reg(rdi);

	u32 offset;
	if constexpr (imm_offset == 1) {
		offset = ((op >> 8 & BITMASK(4)) << 4) | (op & BITMASK(4));
	} else {
		offset = *cpu.get_reg(op & BITMASK(4));
	}

	ADDRESS_WRITE;

	if constexpr (load == 1 && half == 0) {
		*rd = (s8)cpu.nread8(address);
	} else if constexpr (load == 1 && half == 1) {
		int rem = address % 2;
		if constexpr (sign == 1) {
			if (rem) {
				*rd = (s8)cpu.nread8(address);
			} else {
				*rd = (s16)cpu.nread16_noalign(address);
			}
		} else {
			bool c;
			*rd = ror(cpu.nread16_noalign(address), rem * 8, c);
		}
	} else if (load == 0 && half == 1) {
		u16 value = (rni == rdi) ? old_rn : *rd;
		cpu.nwrite16_noalign(address, value & BITMASK(16));
	}

	if constexpr (load == 1) {
		cpu.icycle();
		if (!prefetch_enabled) {
			cpu.nfetch();
		} else {
			cpu.sfetch();
		}
	} else {
		cpu.nfetch();
	}
}

template <u32 prepost, u32 updown, u32 psr, u32 writeback, u32 load>
void arm_block_dt(u32 op)
{
	u32 start_address;

	u32 rni = op >> 16 & BITMASK(4);
	u32 *rn = cpu.get_reg(rni);
	u32 old_rn = *rn;
	u32 register_list = op & BITMASK(16);
	u32 k = std::popcount(register_list) * 4;

	if (register_list == 0) {
		k = 0x40;
	}

	if constexpr (prepost == 0 && updown == 1) {
		start_address = *rn;
		if constexpr (writeback) {
			*rn = *rn + k;
		}
	} else if constexpr (prepost == 1 && updown == 1) {
		start_address = *rn + 4;
		if constexpr (writeback) {
			*rn = *rn + k;
		}
	} else if constexpr (prepost == 0 && updown == 0) {
		start_address = *rn - k + 4;
		if constexpr (writeback) {
			*rn = *rn - k;
		}
	} else {
		start_address = *rn - k;
		if constexpr (writeback) {
			*rn = *rn - k;
		}
	}

	int rem = start_address % 4;
	u32 address = align(start_address, 4);

	bool pc_written = false;

	if constexpr (load == 1 && psr == 0) {
		READ_MULTIPLE(14);

		if (register_list & BIT(15)) {
			pc_written = true;
			if (first) {
				WRITE_PC(cpu.nread32_noalign(address) & 0xFFFF'FFFC);
			} else {
				WRITE_PC(cpu.sread32_noalign(address) & 0xFFFF'FFFC);
			}
		}

	} else if (load == 1 && writeback == 0 && psr == 1 && !(op & BIT(15))) {
		READ_MULTIPLE_FORCE_USER(14);

	} else if (load == 1 && psr == 1 && (op & BIT(15))) {
		READ_MULTIPLE(14);

		cpu.CPSR = *cpu.get_spsr();
		cpu.update_mode();

		u32 mask = cpu.in_thumb_state() ? 0xFFFF'FFFE : 0xFFFF'FFFC;
		pc_written = true;
		if (first) {
			WRITE_PC(cpu.nread32_noalign(address+rem) & mask);
		} else {
			WRITE_PC(cpu.sread32_noalign(address+rem) & mask);
		}

	} else if constexpr (load == 0 && psr == 0) {
		WRITE_MULTIPLE(14);

		if (register_list & BIT(15)) {
			if (first) {
				cpu.nwrite32_noalign(address+rem, cpu.pc + 4);
			} else {
				cpu.swrite32_noalign(address+rem, cpu.pc + 4);
			}
		}

	} else if constexpr (load == 0 && writeback == 0 && psr == 1) {
		WRITE_MULTIPLE_FORCE_USER(14);

		if (register_list & BIT(15)) {
			if (first) {
				cpu.nwrite32_noalign(address+rem, cpu.pc);
			} else {
				cpu.swrite32_noalign(address+rem, cpu.pc);
			}
		}
	}

	if constexpr (load == 0 && writeback == 1) {
		if ((register_list & BITMASK(rni + 1)) == BIT(rni)) {
			cpu.nocycle_write32_noalign(start_address, old_rn);
		}
	}

	if constexpr (load == 1) {
		cpu.icycle();
		if (!prefetch_enabled && !pc_written) {
			cpu.nfetch();
		} else {
			cpu.sfetch();
		}
	} else {
		cpu.nfetch();
	}
}

void arm_swi(u32 op)
{
	(void)op;

	cpu.exception_prologue(SUPERVISOR, 0x13);
	WRITE_PC(VECTOR_SWI);
	cpu.sfetch();
}

void arm_bkpt(u32 op)
{
	(void)op;

	cpu.exception_prologue(ABORT, 0x17);
	WRITE_PC(VECTOR_PREFETCH_ABORT);
	cpu.sfetch();
}
