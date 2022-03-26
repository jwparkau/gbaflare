#include <gba/cpu.h>
#include <gba/arm.h>
#include <gba/thumb.h>
#include <gba/memory.h>
#include <gba/scheduler.h>

#include <stdexcept>
#include <iostream>

struct CPU cpu;

void CPU::reset()
{
}

void CPU::fakeboot()
{
	registers[0][0] = 0x0800'0000;
	registers[0][1] = 0xEA;
	pc = CARTRIDGE_START;
	CPSR = 0x6000'001F;
	for (int i = 0; i < NUM_MODES; i++) {
		registers[i][13] = 0x03007F00;
	}
}

void CPU::flush_pipeline()
{
	if (in_thumb_state()) {
		pc -= 2;
	} else {
		pc -= 4;
	}
	nfetch();
	sfetch();
}

void CPU::step()
{
	u16 inter_enable = io_read<u16>(IO_IE) & BITMASK(14);
	u16 inter_flag = io_read<u16>(IO_IF) & BITMASK(14);

	if (halted) {
		if (inter_enable & inter_flag) {
			halted = false;
		} else {
			cpu_cycles = next_event;
			return;
		}
	}


	if (!(CPSR & IRQ_DISABLE) && (io_read<u8>(IO_IME) & 1)) {
		if (inter_enable & inter_flag) {
			registers[IRQ][14] = pc - (in_thumb_state() ? 4 : 8) + 4;
			SPSR[IRQ] = CPSR;
			CPSR = (CPSR & ~BITMASK(5)) | 0x12;
			update_mode();
			set_flag(T_STATE, 0);
			set_flag(IRQ_DISABLE, 1);
			pc = VECTOR_IRQ;
			flush_pipeline();
			sfetch();
		}
	}

	execute();
}

void CPU::arm_nfetch()
{
	pc += 4;
	pipeline[0] = pipeline[1];
	pipeline[1] = pipeline[2];
	pipeline[2] = read<u32, FROM_FETCH, NSEQ>(pc);
}

void CPU::arm_sfetch()
{
	pc += 4;
	pipeline[0] = pipeline[1];
	pipeline[1] = pipeline[2];
	pipeline[2] = read<u32, FROM_FETCH, SEQ>(pc);
}

void CPU::thumb_nfetch()
{
	pc += 2;
	pipeline[0] = pipeline[1];
	pipeline[1] = pipeline[2];
	pipeline[2] = read<u16, FROM_FETCH, NSEQ>(pc);
}

void CPU::thumb_sfetch()
{
	pc += 2;
	pipeline[0] = pipeline[1];
	pipeline[1] = pipeline[2];
	pipeline[2] = read<u16, FROM_FETCH, SEQ>(pc);
}

void CPU::nfetch()
{
	if (in_thumb_state()) {
		thumb_nfetch();
	} else {
		arm_nfetch();
	}
}

void CPU::sfetch()
{
	if (in_thumb_state()) {
		thumb_sfetch();
	} else {
		arm_sfetch();
	}
}

void CPU::execute()
{
	if (in_thumb_state()) {
		thumb_execute();
	} else {
		arm_execute();
	}
}

void CPU::thumb_execute()
{
	u16 op = pipeline[0];
	ThumbInstruction *fp = thumb_lut[op >> 6];

	fp(op);
}


bool CPU::cond_triggered(const u32 cond)
{
	const bool N = CPSR & SIGN_FLAG;
	const bool Z = CPSR & ZERO_FLAG;
	const bool C = CPSR & CARRY_FLAG;
	const bool V = CPSR & OVERFLOW_FLAG;

	if (cond == 0xE) [[likely]] {
		return true;
	}

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

void CPU::arm_execute()
{
	u32 op = pipeline[0];
	if (pc < BIOS_END) {
		last_bios_opcode = pipeline[2];
	}

	u32 op1 = op >> 20 & BITMASK(8);
	u32 op2 = op >> 4 & BITMASK(4);

	u32 cond = op >> 28 & BITMASK(4);

	if (cond_triggered(cond)) {
		u32 lut_offset = (op1 << 4) + op2;
		ArmInstruction *fp = arm_lut[lut_offset];
		fp(op);
	} else {
		sfetch();
	}
}

void CPU::switch_mode(cpu_mode_t new_mode)
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

void CPU::exception_prologue(cpu_mode_t mode, u8 flag)
{
	registers[mode][14] = pc - (in_thumb_state() ? 2 : 4);
	SPSR[mode] = CPSR;
	CPSR = (CPSR & ~BITMASK(5)) | flag;
	update_mode();
	set_flag(T_STATE, 0);
	set_flag(IRQ_DISABLE, 1);
}

void CPU::update_mode()
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

u32 *CPU::get_reg(int i)
{
	if (i == 15) {
		return &pc;
	}
	return &registers[cpu_mode][i];
}

u32 *CPU::get_spsr()
{
	if (cpu_mode == USER || cpu_mode == SYSTEM) {
		return &CPSR;
	}
	return &SPSR[cpu_mode];
}

u32 *CPU::get_sp()
{
	return &registers[cpu_mode][13];
}

u32 *CPU::get_lr()
{
	return &registers[cpu_mode][14];
}

void CPU::set_flag(u32 flag, bool x)
{
	if (x) {
		CPSR |= flag;
	} else {
		CPSR &= ~flag;
	}
}

bool CPU::in_thumb_state()
{
	return CPSR & T_STATE;
}

bool CPU::in_privileged_mode()
{
	return cpu_mode != USER;
}

bool CPU::has_spsr()
{
	return !(cpu_mode == USER || cpu_mode == SYSTEM);
}

void CPU::dump_registers()
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

u32 CPU::nread32_noalign(addr_t addr)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		return nread32(addr);
	}

	return nread32(align(addr, 4));
}

u16 CPU::nread16_noalign(addr_t addr)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		return nread16(addr);
	}

	return nread16(align(addr, 2));
}

void CPU::nwrite32_noalign(addr_t addr, u32 data)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		bool c;
		int rem = addr % 4;
		data = ror(data, rem * 8, c);
		nwrite8(addr, data);
	} else {
		nwrite32(align(addr, 4), data);
	}
}

void CPU::nwrite16_noalign(addr_t addr, u16 data)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		bool c;
		int rem = addr % 2;
		data = ror(data, rem * 8, c);
		nwrite8(addr, data);
	} else {
		nwrite16(align(addr, 2), data);
	}
}

u32 CPU::sread32_noalign(addr_t addr)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		return sread32(addr);
	}

	return sread32(align(addr, 4));
}

u16 CPU::sread16_noalign(addr_t addr)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		return sread16(addr);
	}

	return sread16(align(addr, 2));
}

void CPU::swrite32_noalign(addr_t addr, u32 data)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		bool c;
		int rem = addr % 4;
		data = ror(data, rem * 8, c);
		swrite8(addr, data);
	} else {
		swrite32(align(addr, 4), data);
	}
}

void CPU::swrite16_noalign(addr_t addr, u16 data)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		bool c;
		int rem = addr % 2;
		data = ror(data, rem * 8, c);
		swrite8(addr, data);
	} else {
		swrite16(align(addr, 2), data);
	}
}

void CPU::nocycle_write32_noalign(addr_t addr, u32 data)
{
	if (SRAM_START <= addr && addr < 0x1000'0000) {
		bool c;
		int rem = addr % 4;
		data = ror(data, rem * 8, c);
		write<u8, FROM_CPU, NOCYCLES>(addr, data);
	} else {
		write<u32, FROM_CPU, NOCYCLES>(align(addr, 4), data);
	}
}

void CPU::icycle()
{
	icycle(1);
}

void CPU::icycle(int n)
{
	cpu_cycles += n;
	if (prefetch_enabled) {
		prefetch.step(n);
	}
}

DEFINE_READ_WRITE(CPU)
