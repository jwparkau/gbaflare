#ifndef GBAFLARE_ARM_H
#define GBAFLARE_ARM_H

#include "types.h"
#include <array>

constexpr std::size_t ARM_LUT_SIZE = 4096;

typedef void ArmInstruction(u32 op);
typedef std::array<ArmInstruction *, ARM_LUT_SIZE> arm_lut_t;

extern const arm_lut_t arm_lut;

ArmInstruction arm_nop;

#endif
