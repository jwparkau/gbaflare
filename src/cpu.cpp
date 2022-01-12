#include "cpu.h"
#include "arm.h"
#include "thumb.h"
#include "memory.h"
#include "scheduler.h"

#include <stdexcept>
#include <iostream>

struct Cpu cpu;
bool debug = false;

#define OP_BUFFER_SIZE 10
u32 opbuffer[OP_BUFFER_SIZE];
u32 pcbuffer[OP_BUFFER_SIZE];
int opi;

void dump_buffer();

static const bool cond_lut[16][16] = {
	{0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
	{1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
	{0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
	{1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
	{1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
	{0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
	{1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
	{1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1},
	{0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0},
	{1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
	{0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

void dump_buffer()
{
	for (int i = opi-1; i >= 0; i--) {
		printf("PC: %08X    OP: %08X\n", pcbuffer[i], opbuffer[i]);
	}

	for (int i = OP_BUFFER_SIZE-1; i >= opi; i--) {
		printf("PC: %08X    OP: %08X\n", pcbuffer[i], opbuffer[i]);
	}
}

void Cpu::reset()
{
}

void Cpu::fakeboot()
{
	registers[0][0] = 0x0800'0000;
	registers[0][1] = 0xEA;
	pc = CARTRIDGE_START;
	CPSR = 0x6000'001F;
	for (int i = 0; i < NUM_MODES; i++) {
		registers[i][13] = 0x03007F00;
	}
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

#ifdef DEBUG
	opbuffer[opi] = pipeline[0];
	pcbuffer[opi] = pc - (in_thumb_state() ? 4 : 8);

	opi = (opi + 1) % OP_BUFFER_SIZE;
#endif

	if (!(CPSR & IRQ_DISABLE) && (io_read<u8>(IO_IME) & 1)) {
		u16 inter_enable = io_read<u16>(IO_IE) & BITMASK(14);
		u16 inter_flag = io_read<u16>(IO_IF) & BITMASK(14);

		if (inter_enable & inter_flag) {
			registers[IRQ][14] = pc - (in_thumb_state() ? 4 : 8) + 4;
			SPSR[IRQ] = CPSR;
			CPSR = (CPSR & ~BITMASK(5)) | 0x12;
			update_mode();
			set_flag(T_STATE, 0);
			set_flag(IRQ_DISABLE, 1);
			pc = VECTOR_IRQ;
			fetch();
			fetch();
		}
	}

	execute();
	fetch();

	cpu_cycles += 1;
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
#ifdef DEBUG
	if (debug) {
		dump_registers();
		fprintf(stderr, "     %04X\n", op);
	}
#endif

	const u32 lut_offset = op >> 6;
	ThumbInstruction *fp = thumb_lut[lut_offset];
#ifdef DEBUG
	if (fp) {
		fp(op);
	} else {
		fprintf(stderr, "UNHANDLED THUMB %04X at %08X\n", op, pc - 4);
		dump_buffer();
		throw std::runtime_error("unhandled opcode in thumb mode");
	}
#else
	fp(op);
#endif
}


bool Cpu::cond_triggered(u32 cond)
{
	return cond_lut[cond][CPSR >> 28];
}

void Cpu::arm_execute()
{
	const u32 op = pipeline[0];

#ifdef DEBUG
	if (debug) {
		dump_registers();
		fprintf(stderr, " %08X\n", op);
	}
#endif

	//printf("%08X %08X\n", pc - 8, op);

	const u32 op1 = op >> 20 & BITMASK(8);
	const u32 op2 = op >> 4 & BITMASK(4);

	const u32 cond = op >> 28 & BITMASK(4);

	if (cond_triggered(cond)) {
		const u32 lut_offset = (op1 << 4) + op2;
		ArmInstruction *fp = arm_lut[lut_offset];
#ifdef DEBUG
		if (fp) {
			fp(op);
		} else {
			fprintf(stderr, "UNHANDLED %08X\n", op);
			dump_buffer();
			throw std::runtime_error("unhandled opcode");
		}
#else
		fp(op);
#endif
	}
}

void Cpu::switch_mode(cpu_mode_t new_mode)
{
	int src = cpu_mode;
	int dst = new_mode;

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

void Cpu::exception_prologue(cpu_mode_t mode, u8 flag)
{
	registers[mode][14] = pc - (in_thumb_state() ? 2 : 4);
	SPSR[mode] = CPSR;
	CPSR = (CPSR & ~BITMASK(5)) | flag;
	update_mode();
	set_flag(T_STATE, 0);
	set_flag(IRQ_DISABLE, 1);
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
	return &registers[cpu_mode][i];
}

u32 *Cpu::get_spsr()
{
	if (cpu_mode == USER || cpu_mode == SYSTEM) {
		return &CPSR;
	}
	return &SPSR[cpu_mode];
}

u32 *Cpu::get_sp()
{
	return &registers[cpu_mode][13];
}

u32 *Cpu::get_lr()
{
	return &registers[cpu_mode][14];
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
		fprintf(stderr, "%08X ", registers[cpu_mode % NUM_MODES][i]);
	}

	if (in_thumb_state()) {
		fprintf(stderr, "%08X ", pc - 2);
	} else {
		fprintf(stderr, "%08X ", pc - 4);
	}

	fprintf(stderr, "cpsr: %08X |", CPSR);
}
