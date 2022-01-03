#include "arm.h"
#include "cpu.h"

#include <iostream>

constexpr static arm_lut_t init_lut();
constexpr static ArmInstruction *decode_op(u32 op);

constexpr static ArmInstruction *decode_op(u32 op)
{
	return &arm_nop;
}

constexpr static arm_lut_t init_lut()
{
	arm_lut_t ret;

	for (u32 op = 0; op < 4096; op++) {
		ret[op] = decode_op(op);
	}

	return ret;
}

const arm_lut_t arm_lut = init_lut();

void arm_nop(u32 op)
{
	printf("nop\n");
}
