#ifndef GBAFLARE_TYPES_H
#define GBAFLARE_TYPES_H

#include <cstdint>

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

typedef std::int8_t s8;
typedef std::int16_t s16;
typedef std::int32_t s32;
typedef std::int64_t s64;

typedef u32 addr_t;

typedef u32 word_t;
typedef u16 halfword_t;
typedef u8 byte_t;

constexpr std::size_t operator""_KiB(unsigned long long v) {
	return v * 1024;
}

constexpr std::size_t operator""_MiB(unsigned long long v) {
	return v * 1024 * 1024;
}

constexpr std::size_t operator""_GiB(unsigned long long v) {
	return v * 1024 * 1024 * 1024;
}

constexpr u32 BITMASK(u32 i) {
	return (i >= 32) ? 0xFFFF'FFFF : (1 << i) - 1;
}

constexpr u32 BIT(u32 i) {
	return 1 << i;
}

u32 readarr32(u8 *arr, std::size_t offset);
u16 readarr16(u8 *arr, std::size_t offset);
u8 readarr8(u8 *arr, std::size_t offset);

void writearr32(u8 *arr, std::size_t offset, u32 data);
void writearr16(u8 *arr, std::size_t offset, u16 data);
void writearr8(u8 *arr, std::size_t offset, u8 data);

#endif
