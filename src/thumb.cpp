#include "thumb.h"

constexpr static thumb_lut_t init_thumb();
constexpr static ThumbInstruction *decode_op(u32 op);

constexpr static ThumbInstruction *decode_op(u32 op)
{
	return nullptr;
}

constexpr static thumb_lut_t init_thumb()
{
	thumb_lut_t ret;

	for (u32 op = 0; op < THUMB_LUT_SIZE; op++) {
		ret[op] = decode_op(op);
	}

	return ret;
}

const thumb_lut_t thumb_lut = init_thumb();
