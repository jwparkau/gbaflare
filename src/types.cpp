#include "types.h"

u32 readarr32(u8 *arr, std::size_t offset)
{
	u8 a0, a1, a2, a3;

	a0 = arr[offset];
	a1 = arr[offset+1];
	a2 = arr[offset+2];
	a3 = arr[offset+3];

	return (a3 << 24ul) + (a2 << 16ul) + (a1 << 8ul) + a0;
}

u16 readarr16(u8 *arr, std::size_t offset)
{
	u8 a0, a1;

	a0 = arr[offset];
	a1 = arr[offset+1];

	return (a1 << 8ul) + a0;
}

u8 readarr8(u8 *arr, std::size_t offset)
{
	return arr[offset];
}

void writearr32(u8 *arr, std::size_t offset, u32 data)
{
	u8 a0, a1, a2, a3;

	a0 = data & BITMASK(8);
	data >>= 8;
	a1 = data & BITMASK(8);
	data >>= 8;
	a2 = data & BITMASK(8);
	data >>= 8;
	a3 = data & BITMASK(8);

	arr[offset] = a0;
	arr[offset+1] = a1;
	arr[offset+2] = a2;
	arr[offset+3] = a3;
}

void writearr16(u8 *arr, std::size_t offset, u16 data)
{
	u8 a0, a1;

	a0 = data & BITMASK(8);
	data >>= 8;
	a1 = data & BITMASK(8);

	arr[offset] = a0;
	arr[offset+1] = a1;
}

void writearr8(u8 *arr, std::size_t offset, u8 data)
{
	arr[offset] = data;
}
