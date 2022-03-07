#ifndef GBAFLARE_THUMB_H
#define GBAFLARE_THUMB_H

#include <common/types.h>
#include <array>

constexpr std::size_t THUMB_LUT_SIZE = 1024;

typedef void ThumbInstruction(u16 op);
typedef std::array<ThumbInstruction *, THUMB_LUT_SIZE> thumb_lut_t;

extern const thumb_lut_t thumb_lut;

#endif
