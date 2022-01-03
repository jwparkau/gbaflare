#include "cpu.h"
#include "arm.h"
#include "memory.h"

struct Cpu cpu;

void Cpu::reset()
{
	fakeboot();
}

void Cpu::fakeboot()
{

}

void Cpu::step()
{
	/*
	 * execute
	 * pop pipeline
	 * prefetch
	 * tick other hw
	 */
	ArmInstruction *f = arm_lut[0];
	f(0);
}

void Cpu::switch_mode(cpu_mode_t cpu_mode)
{

}

u32 *Cpu::get_reg(int i)
{
	return &registers[cpu_mode % 6][i];
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
