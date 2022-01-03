#ifndef GBAFLARE_CPU_H
#define GBAFLARE_CPU_H

#include "types.h"

enum cpu_mode_t {
	USER,
	FIQ,
	SUPERVISOR,
	ABORT,
	IRQ,
	UNDEFINED,
	SYSTEM
};

enum cpsr_flags {
	T_STATE = BIT(5),
	FIQ_DISABLE = BIT(6),
	IRQ_DISABLE = BIT(7),
	OVERFLOW_FLAG = BIT(28),
	CARRY_FLAG = BIT(29),
	ZERO_FLAG = BIT(30),
	SIGN_FLAG = BIT(31)
};

enum cpsr_masks {
	MODE_MASK = BITMASK(4)
};

enum exception_vectors {
	VECTOR_RESET = 0x0,
	VECTOR_UNDEFINED_INSTRUCTION = 0x04,
	VECTOR_SWI = 0x08,
	VECTOR_PREFETCH_ABORT = 0x0C,
	VECTOR_DATA_ABORT = 0x10,
	VECTOR_ADDRESS_EXCEEDS = 0x14,
	VECTOR_IRQ = 0x18,
	VECTOR_FIQ = 0x1C
};

struct Cpu {
	// registers
	u32 registers[6][16]{};
	u32 CPSR{};
	u32 SPSR[6]{};
	u32 &pc = registers[0][15];

	u32 cycles{};
	u32 pipeline[2]{};
	bool in_thumb_state{};
	cpu_mode_t cpu_mode = USER;



	// functions
	void reset();
	void fakeboot();
	void switch_mode(cpu_mode_t cpu_mode);
	void step();

	u32 cpu_read32(addr_t addr);
	u16 cpu_read16(addr_t addr);
	u8 cpu_read8(addr_t addr);

	void cpu_write32(addr_t addr, u32 data);
	void cpu_write16(addr_t addr, u16 data);
	void cpu_write8(addr_t addr, u8 data);

	u32 *get_reg(int i);
};

extern Cpu cpu;

#endif
