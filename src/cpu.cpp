#include "cpu.h"
#include "arm.h"
#include "memory.h"

#include <stdexcept>
#include <iostream>

struct Cpu cpu;

void Cpu::reset()
{
}

void Cpu::fakeboot()
{
	pc = CARTRIDGE_START;
}

void Cpu::flush_pipeline()
{
	fetch();
	fetch();
}

void Cpu::step()
{
	/*
	 * execute
	 * pop pipeline
	 * prefetch
	 * tick other hw
	 */

	execute();
	fetch();
}

void Cpu::fetch()
{
	arm_fetch();
}

void Cpu::arm_fetch()
{
	u32 op = cpu_read32(pc);

	pipeline[0] = pipeline[1];
	pipeline[1] = op;

	pc += 4;
}

void Cpu::execute()
{
	arm_execute();
}

void Cpu::arm_execute()
{
	const u32 op = pipeline[0];
	fprintf(stderr, "OP: %08X, PC: %08X\n", op, pc);

	const u32 op1 = op >> 20 & BITMASK(8);
	const u32 op2 = op >> 4 & BITMASK(4);

	const u32 cond = op >> 28 & BITMASK(4);

	const bool N = CPSR & SIGN_FLAG;
	const bool Z = CPSR & ZERO_FLAG;
	const bool C = CPSR & CARRY_FLAG;
	const bool V = CPSR & OVERFLOW_FLAG;

	switch (cond) {
		case 0:
			if (!Z)
				return;
			break;
		case 1:
			if (!!Z)
				return;
			break;
		case 2:
			if (!C)
				return;
			break;
		case 3:
			if (!!C)
				return;
			break;
		case 4:
			if (!N)
				return;
			break;
		case 5:
			if (!!N)
				return;
			break;
		case 6:
			if (!V)
				return;
			break;
		case 7:
			if (!!V)
				return;
			break;
		case 8:
			if (!(C && !Z))
				return;
			break;
		case 9:
			if (!(!C || Z))
				return;
			break;
		case 0xA:
			if (!(N == V))
				return;
			break;
		case 0xB:
			if (!(N != V))
				return;
			break;
		case 0xC:
			if (!(!Z && N == V))
				return;
			break;
		case 0xD:
			if (!(Z || N != V))
				return;
			break;
		default:
			break;
	}

	const u32 lut_offset = (op1 << 4) + op2;
	ArmInstruction *fp = arm_lut[lut_offset];
	if (fp) {
		fp(op);
	} else {
		throw std::runtime_error("unhandled opcode");
	}
}

void Cpu::switch_mode(cpu_mode_t cpu_mode)
{
	/*
	 * copy registers
	 * copy pc
	 * copy lr
	 */
}

u32 *Cpu::get_reg(int i)
{
	return &registers[cpu_mode % 6][i];
}

u32 *Cpu::get_spsr()
{
	return &SPSR[cpu_mode % 6];
}

void Cpu::set_flag(u32 flag, bool x)
{
	if (x) {
		CPSR |= flag;
	} else {
		CPSR &= ~flag;
	}
}

u32 Cpu::cpu_read32(addr_t addr)
{
	return Memory::read32(addr);
}

u16 Cpu::cpu_read16(addr_t addr)
{
	return Memory::read16(addr);
}

u8 Cpu::cpu_read8(addr_t addr)
{
	return Memory::read8(addr);
}

void Cpu::cpu_write32(addr_t addr, u32 data)
{
	Memory::write32(addr, data);
}

void Cpu::cpu_write16(addr_t addr, u16 data)
{
	Memory::write16(addr, data);
}

void Cpu::cpu_write8(addr_t addr, u8 data)
{
	Memory::write8(addr, data);
}
