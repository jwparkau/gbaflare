#ifndef GBAFLARE_ARM_H
#define GBAFLARE_ARM_H

#include <common/types.h>
#include <array>

constexpr std::size_t ARM_LUT_SIZE = 4096;

typedef void ArmInstruction(u32 op);
typedef std::array<ArmInstruction *, ARM_LUT_SIZE> arm_lut_t;

extern const arm_lut_t arm_lut;

enum ALUOP {
	OP_AND,
	OP_EOR,
	OP_SUB,
	OP_RSB,
	OP_ADD,
	OP_ADC,
	OP_SBC,
	OP_RSC,
	OP_TST,
	OP_TEQ,
	OP_CMP,
	OP_CMN,
	OP_ORR,
	OP_MOV,
	OP_BIC,
	OP_MVN
};

#endif
