#include "cpu.h"
#include "arm.h"
#include "thumb.h"
#include "memory.h"

#include <stdexcept>
#include <iostream>

struct Cpu cpu;
bool debug = false;

void Cpu::reset()
{
}

void Cpu::fakeboot()
{
	registers[0][0] = 0x0800'0000;
	registers[0][1] = 0xEA;
	pc = CARTRIDGE_START;
	CPSR = 0x6000'001F;
	registers[0][13] = 0x03007F00;
}

void Cpu::flush_pipeline()
{
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
	if (in_thumb_state()) {
		thumb_fetch();
	} else {
		arm_fetch();
	}
}

void Cpu::arm_fetch()
{
	u32 op = cpu_read32(pc);

	pipeline[0] = pipeline[1];
	pipeline[1] = op;

	pc += 4;
}

void Cpu::thumb_fetch()
{
	u16 op = cpu_read16(pc);

	pipeline[0] = pipeline[1];
	pipeline[1] = op;

	pc += 2;
}

void Cpu::execute()
{
	if (in_thumb_state()) {
		thumb_execute();
	} else {
		arm_execute();
	}
}

void Cpu::thumb_execute()
{
	const u16 op = pipeline[0];
	if (debug) {
		dump_registers();
		fprintf(stderr, "     %04X\n", op);
	}

	const u32 lut_offset = op >> 6;
	ThumbInstruction *fp = thumb_lut[lut_offset];
	if (fp) {
		fp(op);
	} else {
		throw std::runtime_error("unhandled opcode");
	}
}

bool Cpu::cond_triggered(u32 cond)
{
	const bool N = CPSR & SIGN_FLAG;
	const bool Z = CPSR & ZERO_FLAG;
	const bool C = CPSR & CARRY_FLAG;
	const bool V = CPSR & OVERFLOW_FLAG;

	switch (cond) {
		case 0:
			return Z;
		case 1:
			return !Z;
		case 2:
			return C;
		case 3:
			return !C;
		case 4:
			return N;
		case 5:
			return !N;
		case 6:
			return V;
		case 7:
			return !V;
		case 8:
			return C && !Z;
		case 9:
			return !(C && !Z);
		case 0xA:
			return N == V;
		case 0xB:
			return !(N == V);
		case 0xC:
			return (!Z && N == V);
		case 0xD:
			return !(!Z && N == V);
		default:
			return true;
	}
}

void Cpu::arm_execute()
{
	const u32 op = pipeline[0];
	if (debug) {
		dump_registers();
		fprintf(stderr, " %08X\n", op);
	}

	const u32 op1 = op >> 20 & BITMASK(8);
	const u32 op2 = op >> 4 & BITMASK(4);

	const u32 cond = op >> 28 & BITMASK(4);

	if (cond_triggered(cond)) {
		const u32 lut_offset = (op1 << 4) + op2;
		ArmInstruction *fp = arm_lut[lut_offset];
		if (fp) {
			fp(op);
		} else {
			throw std::runtime_error("unhandled opcode");
		}
	}
}

void Cpu::switch_mode(cpu_mode_t new_mode)
{
	int src = cpu_mode % 6;
	int dst = new_mode % 6;

	cpu_mode = new_mode;

	if (src == dst) {
		return;
	}

	for (int i = 0; i < 8; i++) {
		registers[0][i] = registers[src][i];
		registers[dst][i] = registers[0][i];
	}

	if (src != FIQ) {
		for (int i = 8; i < 13; i++) {
			registers[0][i] = registers[src][i];
		}
	}
	if (dst != FIQ) {
		for (int i = 8; i < 13; i++) {
			registers[dst][i] = registers[0][i];
		}
	}
}

void Cpu::update_mode()
{
	cpu_mode_t new_mode;

	switch (CPSR & BITMASK(5)) {
		case 0x10:
			new_mode = USER;
			break;
		case 0x11:
			new_mode = FIQ;
			break;
		case 0x12:
			new_mode = IRQ;
			break;
		case 0x13:
			new_mode = SUPERVISOR;
			break;
		case 0x17:
			new_mode = ABORT;
			break;
		case 0x1B:
			new_mode = UNDEFINED;
			break;
		case 0x1F:
			new_mode = SYSTEM;
			break;
		default:
			throw std::runtime_error("unhandled mode bits?");
	}

	switch_mode(new_mode);
}

u32 *Cpu::get_reg(int i)
{
	if (i == 15) {
		return &pc;
	}
	return &registers[cpu_mode % 6][i % 16];
}

u32 *Cpu::get_spsr()
{
	return &SPSR[cpu_mode % 6];
}

u32 *Cpu::get_sp()
{
	return &registers[cpu_mode % 6][13];
}

u32 *Cpu::get_lr()
{
	return &registers[cpu_mode % 6][14];
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

bool Cpu::in_thumb_state()
{
	return CPSR & T_STATE;
}

bool Cpu::in_privileged_mode()
{
	return cpu_mode != USER;
}

bool Cpu::has_spsr()
{
	return !(cpu_mode == USER || cpu_mode == SYSTEM);
}

void Cpu::dump_registers()
{
	for (int i = 0; i < 15; i++) {
		fprintf(stderr, "%08X ", registers[0][i]);
	}

	if (in_thumb_state()) {
		fprintf(stderr, "%08X ", pc - 2);
	} else {
		fprintf(stderr, "%08X ", pc - 4);
	}

	fprintf(stderr, "cpsr: %08X |", CPSR);
}
