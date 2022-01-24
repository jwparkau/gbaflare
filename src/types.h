#ifndef GBAFLARE_TYPES_H
#define GBAFLARE_TYPES_H

#include <cstdint>
#include <cstring>
#include <bit>

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

inline u32 align(u32 addr, u32 x)
{
	return addr / x * x;
}

inline int min(int a, int b) {
	return (a <= b) ? a : b;
}

inline int max(int a, int b) {
	return (a >= b) ? a : b;
}

inline int at_least(int x, int n) {
	return max(x, n);
}

inline int at_most(int x, int n) {
	return min(x, n);
}

u32 ror(u32 x, u32 n, bool &carry);
u32 ror(u32 x, u32 n);
u32 asr(u32 x, u32 n, bool &carry);
u32 lsr(u32 x, u32 n, bool &carry);
u32 lsl(u32 x, u32 n, bool &carry);
u32 rrx(u32 x, bool &carry);

#define GET_FLAG(x, f) ((x) >> f##_SHIFT & f##_MASK)
#define SET_FLAG(x, f, v) ((x) = ((x) & ~(f##_MASK << f##_SHIFT)) | (v << f##_SHIFT))

#define DECLARE_READ_WRITE \
u32 read32(addr_t addr);\
u16 read16(addr_t addr);\
u8 read8(addr_t addr);\
void write32(addr_t addr, u32 data);\
void write16(addr_t addr, u16 data);\
void write8(addr_t addr, u8 data)

#define DEFINE_READ_WRITE(x) \
u32 x::read32(addr_t addr) { return read<u32, FROM_##x>(addr); }\
u16 x::read16(addr_t addr) { return read<u16, FROM_##x>(addr); }\
u8 x::read8(addr_t addr) { return read<u8, FROM_##x>(addr); }\
void x::write32(addr_t addr, u32 data) { write<u32, FROM_##x>(addr, data); }\
void x::write16(addr_t addr, u16 data) { write<u16, FROM_##x>(addr, data); }\
void x::write8(addr_t addr, u8 data) { write<u8, FROM_##x>(addr, data); }

template<typename T> T readarr(u8 *arr, u32 offset)
{
	if constexpr (std::endian::native == std::endian::little) {
		T x;
		memcpy(&x, arr + offset, sizeof(T));

		return x;
	} else {
		u8 a0 = arr[offset];

		if constexpr (sizeof(T) == sizeof(u8)) {
			return a0;
		}

		u8 a1 = arr[offset+1];

		if constexpr (sizeof(T) == sizeof(u16)) {
			return (a1 << 8) | a0;
		}

		u8 a2 = arr[offset+2];
		u8 a3 = arr[offset+3];

		return (a3 << 24) | (a2 << 16) | (a1 << 8) | a0;
	}
}

template<typename T> void writearr(u8 *arr, u32 offset, T data)
{
	if constexpr (std::endian::native == std::endian::little) {
		memcpy(arr + offset, &data, sizeof(T));
	} else {
		u32 x = data;
		u32 mask = BITMASK(8);

		arr[offset] = x & mask;

		if constexpr (sizeof(T) == sizeof(u8)) {
			return;
		}

		x >>= 8;
		arr[offset+1] = x & mask;

		if constexpr (sizeof(T) == sizeof(u16)) {
			return;
		}

		x >>= 8;
		arr[offset+2] = x & mask;
		x >>= 8;
		arr[offset+3] = x & mask;
	}
}

#endif
