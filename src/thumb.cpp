#include "thumb.h"
#include "cpu.h"
#include "ops.h"

#include <bit>
#include <iostream>

constexpr static thumb_lut_t init_thumb();
constexpr static ThumbInstruction *decode_op(u32 op);
void thumb_branch_cond(u16 op);
template <u32 h> void thumb_branch(u16 op);
template <u32 shift_type> void thumb_shift_reg(u16 op);
template <u32 aluop> void thumb_addsub(u16 op);
template <u32 aluop> void thumb_addsubcmpmov(u16 op);
template <u32 aluop> void thumb_alu(u16 op);
void thumb_bx(u16 op);
template <u32 aluop> void thumb_special_data(u16 op);
void thumb_load_pool(u16 op);
template <u32 code> void thumb_loadstore_reg(u16 op);
template <u32 code> void thumb_loadstore_imm(u16 op);
template <u32 code> void thumb_loadstore_half(u16 op);
template <u32 code> void thumb_loadstore_sp(u16 op);
template <u32 code> void thumb_addpcsp(u16 op);
template <u32 code> void thumb_sp_add(u16 op);
template <u32 code, u32 pclr> void thumb_pushpop(u16 op);
template <u32 code> void thumb_multiple(u16 op);
void thumb_swi(u16 op);

constexpr static ThumbInstruction *decode_op(u32 op)
{
	if (op >> 6 == 0xD && (op >> 3 & BITMASK(3)) != 7) {
		return thumb_branch_cond;
	}

	if (op >> 7 == 7) {
		u32 h = op >> 5 & BITMASK(2);
#include "gencpp/THUMB_BRANCH.gencpp"
	}

	if (op >> 7 == 0 && op >> 5 != 3) {
		u32 shift_type = op >> 5 & BITMASK(2);
#include "gencpp/THUMB_SHIFT_REG.gencpp"
	}

	if (op >> 5 == 3) {
		u32 aluop = op >> 3 & BITMASK(2);
#include "gencpp/THUMB_ADDSUB.gencpp"
	}

	if (op >> 7 == 1) {
		u32 aluop = op >> 5 & BITMASK(2);
#include "gencpp/THUMB_ADDSUBCMPMOV.gencpp"
	}

	if (op >> 4 == 16) {
		u32 aluop = op & BITMASK(4);
#include "gencpp/THUMB_ALU.gencpp"
	}

	if (op >> 1 == 0x8E) {
		return thumb_bx;
	}

	if (op >> 4 == 17) {
		u32 aluop = op >> 2 & BITMASK(2);
#include "gencpp/THUMB_SPECIAL.gencpp"
	}

	if (op >> 5 == 9) {
		return thumb_load_pool;
	}

	if (op >> 6 == 5) {
		u32 code = op >> 3 & BITMASK(3);
#include "gencpp/THUMB_LOADSTORE_REG.gencpp"
	}

	if (op >> 7 == 3) {
		u32 code = op >> 5 & BITMASK(2);
#include "gencpp/THUMB_LOADSTORE_IMM.gencpp"
	}

	if (op >> 6 == 8) {
		u32 code = op >> 5 & BITMASK(1);
#include "gencpp/THUMB_LOADSTORE_HALF.gencpp"
	}

	if (op >> 6 == 9) {
		u32 code = op >> 5 & BITMASK(1);
#include "gencpp/THUMB_LOADSTORE_SP.gencpp"
	}

	if (op >> 6 == 10) {
		u32 code = op >> 5 & BITMASK(1);
#include "gencpp/THUMB_ADDPCSP.gencpp"
	}

	if (op >> 2 == 0xB0) {
		u32 code = op >> 1 & BITMASK(1);
#include "gencpp/THUMB_SPADD.gencpp"
	}

	if (op >> 6 == 0xB && (op >> 3 & BITMASK(2)) == 2) {
		u32 code = op >> 5 & BITMASK(1);
		u32 pclr = op >> 2 & BITMASK(2);
#include "gencpp/THUMB_PUSHPOP.gencpp"
	}

	if (op >> 6 == 0xC) {
		u32 code = op >> 5 & BITMASK(1);
#include "gencpp/THUMB_MULTIPLE.gencpp"
	}

	if (op >> 2 == 0xDF) {
		return thumb_swi;
	}

	return nullptr;
}

constexpr static thumb_lut_t init_thumb()
{
	thumb_lut_t ret;

	for (u32 op = 0; op < THUMB_LUT_SIZE; op++) {
		ret[op] = decode_op(op);
	}

	return ret;
}

const thumb_lut_t thumb_lut = init_thumb();

void thumb_branch_cond(u16 op)
{
	u32 cond = op >> 8 & BITMASK(4);
	s8 imm = op & BITMASK(8);

	if (cpu.cond_triggered(cond)) {
		cpu.pc = cpu.pc + (s32)imm * 2;
		cpu.flush_pipeline();
	}
}

template <u32 h>
void thumb_branch(u16 op)
{
	u32 imm = op & BITMASK(11);
	s32 nn = (s32)(imm << 21) >> 21;

	u32 *lr = cpu.get_lr();

	if constexpr (h == 0) {
		WRITE_PC(cpu.pc + nn * 2);
	} else if constexpr (h == 2) {
		*lr = cpu.pc + (nn << 12);
	} else if constexpr (h == 3) {
		u32 old_pc = cpu.pc;
		WRITE_PC(*lr + imm * 2);
		*lr = (old_pc - 2) | 1;
	}
}

void thumb_bx(u16 op)
{
	u32 rm = *cpu.get_reg(op >> 3 & BITMASK(4));

	BRANCH_X_RM;
}

template <u32 shift_type>
void thumb_shift_reg(u16 op)
{
	GET_CPU_FLAGS;

	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 rm = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 imm = op >> 6 & BITMASK(5);

	BARREL_SHIFTER(*rd, C);

	u32 r = *rd;
	NZ_FLAGS_RD;
	WRITE_CPU_FLAGS;
}

template <u32 aluop>
void thumb_addsub(u16 op)
{
	GET_CPU_FLAGS;

	u32 operand;

	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));

	if constexpr (aluop == 0 || aluop == 1) {
		operand = *cpu.get_reg(op >> 6 & BITMASK(3));
	} else {
		operand = op >> 6 & BITMASK(3);
	}

	u32 r;
	if constexpr (aluop == 0 || aluop == 2) {
		u64 result = (u64)rn + operand;
		r = *rd = result;
		CV_FLAGS_ADD;
	} else if constexpr (aluop == 1 || aluop == 3) {
		r = *rd = rn - operand;
		CV_FLAGS_SUB;
	}

	NZ_FLAGS_RD;

	WRITE_CPU_FLAGS;
}

template <u32 aluop>
void thumb_addsubcmpmov(u16 op)
{
	GET_CPU_FLAGS;

	u32 *rd = cpu.get_reg(op >> 8 & BITMASK(3));
	u32 operand = op & BITMASK(8);

	u32 rn = *rd;

	u32 r;
	if constexpr (aluop == 0) {
		r = *rd = operand;
	} else if constexpr (aluop == 1) {
		r = rn - operand;
		CV_FLAGS_SUB;
	} else if constexpr (aluop == 2) {
		u64 result = (u64)rn + operand;
		r = *rd = result;
		CV_FLAGS_ADD;
	} else if constexpr (aluop == 3) {
		r = *rd = rn - operand;
		CV_FLAGS_SUB;
	}

	NZ_FLAGS_RD;

	WRITE_CPU_FLAGS;
}

template <u32 aluop>
void thumb_alu(u16 op)
{
	GET_CPU_FLAGS;

	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 operand = *cpu.get_reg(op >> 3 & BITMASK(3));

	u32 rn = *rd;
	u32 r;

	if constexpr (aluop == 0) {
		r = *rd = rn & operand;
	} else if constexpr (aluop == 1) {
		r = *rd = rn ^ operand;
	} else if constexpr (aluop == 2) {
		r = *rd = lsl(rn, operand & 0xFF, C);
	} else if constexpr (aluop == 3) {
		r = *rd = lsr(rn, operand & 0xFF, C);
	} else if constexpr (aluop == 4) {
		r = *rd = asr(rn, operand & 0xFF, C);
	} else if constexpr (aluop == 5) {
		u64 result = (u64)rn + operand + C;
		r = *rd = result;
		CV_FLAGS_ADD;
	} else if constexpr (aluop == 6) {
		s64 result = (s64)rn - operand - (!C);
		r = *rd = result;
		C = !(result < 0);
		V = (rn ^ operand) & (rn ^ r) & 0x8000'0000;
	} else if constexpr (aluop == 7) {
		r = *rd = ror(rn, operand & 0xFF, C);
	} else if constexpr (aluop == 8) {
		r = rn & operand;
	} else if constexpr (aluop == 9) {
		rn = 0;
		r = *rd = rn - operand;
		CV_FLAGS_SUB;
	} else if constexpr (aluop == 0xA) {
		r = rn - operand;
		CV_FLAGS_SUB;
	} else if constexpr (aluop == 0xB) {
		u64 result = (u64)rn + operand;
		r = result;
		CV_FLAGS_ADD;
	} else if constexpr (aluop == 0xC) {
		r = *rd = rn | operand;
	} else if constexpr (aluop == 0xD) {
		r = *rd = rn * operand;	
	} else if constexpr (aluop == 0xE) {
		r = *rd = rn & (~operand);
	} else if constexpr (aluop == 0xF) {
		r = *rd = ~operand;
	}

	NZ_FLAGS_RD;

	WRITE_CPU_FLAGS;
}

template <u32 aluop>
void thumb_special_data(u16 op)
{
	u32 rdi = op & BITMASK(3);
	if (op & BIT(7)) {
		rdi += 8;
	}

	u32 *rd = cpu.get_reg(rdi);
	u32 operand = *cpu.get_reg(op >> 3 & BITMASK(4));

	bool pc_written = false;

	if constexpr (aluop == 0) {
		*rd = *rd + operand;
		if (rdi == 15) {
			pc_written = true;
		}
	} else if constexpr (aluop == 1) {
		GET_CPU_FLAGS;

		u32 rn = *rd;
		u32 r = rn - operand;
		NZ_FLAGS_RD;
		CV_FLAGS_SUB;

		WRITE_CPU_FLAGS;
	} else if constexpr (aluop == 2) {
		*rd = operand;
		if (rdi == 15) {
			pc_written = true;
		}
	}

	if (pc_written) {
		*rd = align(*rd, 2);
	}
}

void thumb_load_pool(u16 op)
{
	u32 *rd = cpu.get_reg(op >> 8 & BITMASK(3));
	u32 nn = op & BITMASK(8);

	*rd = cpu.cpu_read32(align(cpu.pc, 4) + nn * 4);
}

template <u32 code>
void thumb_loadstore_reg(u16 op)
{
	u32 rm = *cpu.get_reg(op >> 6 & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 *rd = cpu.get_reg(op & BITMASK(3));

	u32 address = rn + rm;

	if constexpr (code == 0) {
		cpu.cpu_write32(align(address, 4), *rd);
	} else if constexpr (code == 1) {
		cpu.cpu_write16(align(address, 2), *rd & BITMASK(16));
	} else if constexpr (code == 2) {
		cpu.cpu_write8(address, *rd & BITMASK(8));
	} else if constexpr (code == 3) {
		*rd = (s8)cpu.cpu_read8(address);
	} else if constexpr (code == 4) {
		*rd = cpu.cpu_read32(align(address, 4));
	} else if constexpr (code == 5) {
		*rd = cpu.cpu_read16(align(address, 2));
	} else if constexpr (code == 6) {
		*rd = cpu.cpu_read8(address);
	} else if constexpr (code == 7) {
		*rd = (s16)cpu.cpu_read16(align(address, 2));
	}
}

template <u32 code>
void thumb_loadstore_imm(u16 op)
{
	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 imm = op >> 6 & BITMASK(5);

	if constexpr (code == 0) {
		u32 address = rn + imm * 4;
		cpu.cpu_write32(align(address, 4), *rd);
	} else if constexpr (code == 1) {
		u32 address = rn + imm * 4;
		*rd = cpu.cpu_read32(align(address, 4));
	} else if constexpr (code == 2) {
		u32 address = rn + imm;
		cpu.cpu_write8(address, *rd & BITMASK(8));
	} else if constexpr (code == 3) {
		u32 address = rn + imm;
		*rd = cpu.cpu_read8(address);
	}
}

template <u32 code>
void thumb_loadstore_half(u16 op)
{
	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 imm = op >> 6 & BITMASK(5);

	u32 address = rn + imm * 2;

	if constexpr (code == 0) {
		cpu.cpu_write16(align(address, 2), *rd & BITMASK(16));
	} else if constexpr (code == 1) {
		*rd = cpu.cpu_read16(align(address, 2));
	}
}

template <u32 code>
void thumb_loadstore_sp(u16 op)
{
	u32 imm = op & BITMASK(8);
	u32 *rd = cpu.get_reg(op >> 8 & BITMASK(3));

	u32 sp = *cpu.get_sp();
	u32 address = sp + imm * 4;

	if constexpr (code == 0) {
		cpu.cpu_write32(align(address, 4), *rd);
	} else if constexpr (code == 1) {
		*rd = cpu.cpu_read32(align(address, 4));
	}
}

template <u32 code>
void thumb_addpcsp(u16 op)
{
	u32 imm = op & BITMASK(8);
	u32 *rd = cpu.get_reg(op >> 8 & BITMASK(3));

	if constexpr (code == 0) {
		*rd = (cpu.pc & 0xFFFF'FFFC) + (imm << 2);
	} else if constexpr (code == 1) {
		*rd = *cpu.get_sp() + (imm << 2);
	}
}

template <u32 code>
void thumb_sp_add(u16 op)
{
	u32 imm = op & BITMASK(7);
	u32 *sp = cpu.get_sp();

	if constexpr (code == 0) {
		*sp = *sp + (imm << 2);
	} else if constexpr (code == 1) {
		*sp = *sp - (imm << 2);
	}
}

template <u32 code, u32 pclr> 
void thumb_pushpop(u16 op)
{
	u32 register_list = op & BITMASK(8);
	u32 *sp = cpu.get_sp();

	u32 k = 4 * (std::popcount(register_list) + pclr);

	if constexpr (code == 0) {
		u32 address = align(*sp - k, 4);

		WRITE_MULTIPLE(7);

		if constexpr (pclr) {
			cpu.cpu_write32(address, *cpu.get_lr());
		}

		*sp = *sp - k;
	} else if constexpr (code == 1) {
		u32 end_address = *sp + k;
		u32 address = align(*sp, 4);

		READ_MULTIPLE(7);

		if constexpr (pclr) {
			WRITE_PC(align(cpu.cpu_read32(address), 2));
		}

		*sp = end_address;
	}
}

template <u32 code>
void thumb_multiple(u16 op)
{
	u32 register_list = op & BITMASK(8);
	u32 *rn = cpu.get_reg(op >> 8 & BITMASK(3));

	u32 k = 4 * std::popcount(register_list);
	u32 address = align(*rn, 4);

	if constexpr (code == 0) {
		WRITE_MULTIPLE(7);
	} else if constexpr (code == 1) {
		READ_MULTIPLE(7);
	}

	*rn = *rn + k;
}

void thumb_swi(u16 op)
{
	EXCEPTION_PROLOGUE(SUPERVISOR, 0x13);
	WRITE_PC(VECTOR_SWI);
}
