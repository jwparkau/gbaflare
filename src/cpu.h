#ifndef GBAFLARE_CPU_H
#define GBAFLARE_CPU_H

#include "types.h"

// do not reorder
enum cpu_mode_t {
	SYSTEM,
	FIQ,
	SUPERVISOR,
	ABORT,
	IRQ,
	UNDEFINED,
	USER
};

#define NUM_MODES 7

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

struct CPU {
	// registers
	u32 registers[NUM_MODES][16]{};
	u32 CPSR{};
	u32 SPSR[NUM_MODES]{};
	u32 pc{};
	u32 pipeline[3]{};

	bool halted{};

	cpu_mode_t cpu_mode = SYSTEM;

	// functions
	void reset();
	void fakeboot();
	void flush_pipeline();
	void switch_mode(cpu_mode_t new_mode);
	void update_mode();
	void exception_prologue(cpu_mode_t new_mode, u8 flag);

	void step();

	void fetch();
	void arm_fetch();
	void thumb_fetch();
	void execute();
	void arm_execute();
	void thumb_execute();

	DECLARE_READ_WRITE;

	u32 nread32_noalign(addr_t addr);
	u16 nread16_noalign(addr_t addr);
	void nwrite32_noalign(addr_t addr, u32 data);
	void nwrite16_noalign(addr_t addr, u16 data);
	u32 sread32_noalign(addr_t addr);
	u16 sread16_noalign(addr_t addr);
	void swrite32_noalign(addr_t addr, u32 data);
	void swrite16_noalign(addr_t addr, u16 data);

	u32 *get_reg(int i);
	u32 *get_spsr();
	u32 *get_sp();
	u32 *get_lr();

	void set_flag(u32 flag, bool x);

	bool in_thumb_state();
	bool in_privileged_mode();
	bool has_spsr();

	void dump_registers();

	bool cond_triggered(u32 cond);
};

extern CPU cpu;

extern bool debug;


#endif
