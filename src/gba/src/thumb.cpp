#include <gba/thumb.h>
#include <gba/cpu.h>
#include <gba/ops.h>
#include <gba/memory.h>

#include <bit>
#include <iostream>

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

#include <gba/cpu/thumb_lut.cpp>

void thumb_branch_cond(u16 op)
{
	u32 cond = op >> 8 & BITMASK(4);
	s8 imm = op & BITMASK(8);

	if (cpu.cond_triggered(cond)) {
		WRITE_PC(cpu.pc + (s32)imm * 2);
	}
	cpu.sfetch();
}

template <u32 h>
void thumb_branch(u16 op)
{
	u32 imm = op & BITMASK(11);
	s32 nn = (s32)(imm << 21) >> 21;

	if constexpr (h == 0) {
		WRITE_PC(cpu.pc + nn * 2);
	} else if constexpr (h == 2) {
		u32 *lr = cpu.get_lr();
		*lr = cpu.pc + (nn << 12);
	} else if constexpr (h == 3) {
		u32 *lr = cpu.get_lr();
		u32 old_pc = cpu.pc;
		WRITE_PC(*lr + imm * 2);
		*lr = (old_pc - 2) | 1;
	}
	cpu.sfetch();
}

void thumb_bx(u16 op)
{
	u32 rm = *cpu.get_reg(op >> 3 & BITMASK(4));

	BRANCH_X_RM;
	cpu.sfetch();
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
	cpu.sfetch();
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
	cpu.sfetch();
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
	cpu.sfetch();
}

template <u32 aluop>
void thumb_alu(u16 op)
{
	GET_CPU_FLAGS;

	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 operand = *cpu.get_reg(op >> 3 & BITMASK(3));

	u32 rn = *rd;
	u32 rs = rn;
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
		MUL_ONES_ZEROS;
		cpu.icycle(m);
	} else if constexpr (aluop == 0xE) {
		r = *rd = rn & (~operand);
	} else if constexpr (aluop == 0xF) {
		r = *rd = ~operand;
	}

	NZ_FLAGS_RD;

	WRITE_CPU_FLAGS;

	if constexpr (aluop == 2 || aluop == 3 || aluop == 4 || aluop == 7) {
		cpu.icycle();
	}

	if constexpr (aluop == 2 || aluop == 3 || aluop == 4 || aluop == 7 || aluop == 0xD) {
		if (!prefetch_enabled) {
			cpu.nfetch();
		} else {
			cpu.sfetch();
		}
	} else {
		cpu.sfetch();
	}
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
		cpu.flush_pipeline();
	}
	cpu.sfetch();
}

void thumb_load_pool(u16 op)
{
	u32 *rd = cpu.get_reg(op >> 8 & BITMASK(3));
	u32 nn = op & BITMASK(8);

	*rd = cpu.nread32_noalign(cpu.pc + nn * 4);
	cpu.icycle();
	if (!prefetch_enabled) {
		cpu.nfetch();
	} else {
		cpu.sfetch();
	}
}

template <u32 code>
void thumb_loadstore_reg(u16 op)
{
	u32 rm = *cpu.get_reg(op >> 6 & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 *rd = cpu.get_reg(op & BITMASK(3));

	u32 address = rn + rm;

	if constexpr (code == 0) {
		cpu.nwrite32_noalign(address, *rd);
	} else if constexpr (code == 1) {
		cpu.nwrite16_noalign(address, *rd & BITMASK(16));
	} else if constexpr (code == 2) {
		cpu.nwrite8(address, *rd & BITMASK(8));
	} else if constexpr (code == 3) {
		*rd = (s8)cpu.nread8(address);
	} else if constexpr (code == 4) {
		bool c;
		int rem = address % 4;
		*rd = ror(cpu.nread32_noalign(address), rem * 8, c);
	} else if constexpr (code == 5) {
		bool c;
		int rem = address % 2;
		*rd = ror(cpu.nread16_noalign(address), rem * 8, c);
	} else if constexpr (code == 6) {
		*rd = cpu.nread8(address);
	} else if constexpr (code == 7) {
		u32 rem;

		rem = address & BITMASK(1);

		if (rem) {
			*rd = (s8)cpu.nread8(address);
		} else {
			*rd = (s16)cpu.nread16_noalign(address);
		}
	}

	if constexpr (code >= 3) {
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

template <u32 code>
void thumb_loadstore_imm(u16 op)
{
	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 imm = op >> 6 & BITMASK(5);

	if constexpr (code == 0) {
		u32 address = rn + imm * 4;
		cpu.nwrite32_noalign(address, *rd);
	} else if constexpr (code == 1) {
		u32 address = rn + imm * 4;
		bool c;
		int rem = address % 4;
		*rd = ror(cpu.nread32_noalign(address), rem * 8, c);
	} else if constexpr (code == 2) {
		u32 address = rn + imm;
		cpu.nwrite8(address, *rd & BITMASK(8));
	} else if constexpr (code == 3) {
		u32 address = rn + imm;
		*rd = cpu.nread8(address);
	}

	if constexpr (code % 2 == 1) {
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

template <u32 code>
void thumb_loadstore_half(u16 op)
{
	u32 *rd = cpu.get_reg(op & BITMASK(3));
	u32 rn = *cpu.get_reg(op >> 3 & BITMASK(3));
	u32 imm = op >> 6 & BITMASK(5);

	u32 address = rn + imm * 2;

	if constexpr (code == 0) {
		cpu.nwrite16_noalign(address, *rd & BITMASK(16));
	} else if constexpr (code == 1) {
		bool c;
		int rem = address % 2;
		*rd = ror(cpu.nread16_noalign(address), rem * 8, c);
	}

	if constexpr (code % 2 == 1) {
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

template <u32 code>
void thumb_loadstore_sp(u16 op)
{
	u32 imm = op & BITMASK(8);
	u32 *rd = cpu.get_reg(op >> 8 & BITMASK(3));

	u32 sp = *cpu.get_sp();
	u32 address = sp + imm * 4;

	if constexpr (code == 0) {
		cpu.nwrite32_noalign(address, *rd);
	} else if constexpr (code == 1) {
		bool c;
		int rem = address % 4;
		*rd = ror(cpu.nread32_noalign(address), rem * 8, c);
	}

	if constexpr (code % 2 == 1) {
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

	cpu.sfetch();
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

	cpu.sfetch();
}

template <u32 code, u32 pclr>
void thumb_pushpop(u16 op)
{
	u32 register_list = op & BITMASK(8);
	u32 *sp = cpu.get_sp();

	u32 k = 4 * (std::popcount(register_list) + pclr);

	bool pc_written = false;

	if constexpr (code == 0) {
		int rem = (*sp - k) % 4;
		u32 address = align(*sp - k, 4);

		WRITE_MULTIPLE(7);

		if constexpr (pclr) {
			if (first) {
				cpu.nwrite32_noalign(address+rem, *cpu.get_lr());
			} else {
				cpu.swrite32_noalign(address+rem, *cpu.get_lr());
			}
		}

		*sp = *sp - k;
	} else if constexpr (code == 1) {
		int rem = *sp % 4;
		u32 address = align(*sp, 4);
		u32 end_address = *sp + k;

		READ_MULTIPLE(7);

		if constexpr (pclr) {
			pc_written = true;
			if (first) {
				WRITE_PC(align(cpu.nread32_noalign(address+rem), 2));
			} else {
				WRITE_PC(align(cpu.sread32_noalign(address+rem), 2));
			}
		}

		*sp = end_address;
	}

	if constexpr (code == 1) {
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

template <u32 code>
void thumb_multiple(u16 op)
{
	u32 register_list = op & BITMASK(8);
	u32 rni = op >> 8 & BITMASK(3);
	u32 *rn = cpu.get_reg(rni);
	u32 old_rn = *rn;

	u32 k = 4 * std::popcount(register_list);
	u32 start_address = *rn;
	u32 address = align(start_address, 4);
	int rem = start_address % 4;

	if (register_list == 0) {
		k = 0x40;
	}

	*rn = *rn + k;

	bool pc_written = false;

	if constexpr (code == 0) {
		WRITE_MULTIPLE(7);
	} else if constexpr (code == 1) {
		READ_MULTIPLE(7);
	}

	if constexpr (code == 0) {
		if ((register_list & BITMASK(rni + 1)) == BIT(rni)) {
			cpu.nocycle_write32_noalign(start_address, old_rn);
		}
	} else {
		if (register_list & BIT(rni)) {
			*rn = old_rn;
		}
	}

	if constexpr (code == 1) {
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

void thumb_swi(u16 op)
{
	(void)op;

	cpu.exception_prologue(SUPERVISOR, 0x13);
	WRITE_PC(VECTOR_SWI);
	cpu.sfetch();
}
