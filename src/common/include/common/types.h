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

#define LCD_WIDTH 240l
#define LCD_HEIGHT 160l
#define FRAMEBUFFER_SIZE 38400l


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

#define RESET_ARR(a, n)\
	for (int i = 0; i < (n); i++) {\
		a[i] = {};\
	}

#define ZERO_ARR(a) std::memset(a, 0, sizeof(a))

#define DECLARE_READ_WRITE \
u32 nread32(addr_t addr);\
u16 nread16(addr_t addr);\
u8 nread8(addr_t addr);\
void nwrite32(addr_t addr, u32 data);\
void nwrite16(addr_t addr, u16 data);\
void nwrite8(addr_t addr, u8 data);\
u32 sread32(addr_t addr);\
u16 sread16(addr_t addr);\
u8 sread8(addr_t addr);\
void swrite32(addr_t addr, u32 data);\
void swrite16(addr_t addr, u16 data);\
void swrite8(addr_t addr, u8 data)

#define DEFINE_READ_WRITE(x) \
u32 x::nread32(addr_t addr) { return read<u32, FROM_##x, NSEQ>(addr); }\
u16 x::nread16(addr_t addr) { return read<u16, FROM_##x, NSEQ>(addr); }\
u8 x::nread8(addr_t addr) { return read<u8, FROM_##x, NSEQ>(addr); }\
void x::nwrite32(addr_t addr, u32 data) { write<u32, FROM_##x, NSEQ>(addr, data); }\
void x::nwrite16(addr_t addr, u16 data) { write<u16, FROM_##x, NSEQ>(addr, data); }\
void x::nwrite8(addr_t addr, u8 data) { write<u8, FROM_##x, NSEQ>(addr, data); }\
u32 x::sread32(addr_t addr) { return read<u32, FROM_##x, SEQ>(addr); }\
u16 x::sread16(addr_t addr) { return read<u16, FROM_##x, SEQ>(addr); }\
u8 x::sread8(addr_t addr) { return read<u8, FROM_##x, SEQ>(addr); }\
void x::swrite32(addr_t addr, u32 data) { write<u32, FROM_##x, SEQ>(addr, data); }\
void x::swrite16(addr_t addr, u16 data) { write<u16, FROM_##x, SEQ>(addr, data); }\
void x::swrite8(addr_t addr, u8 data) { write<u8, FROM_##x, SEQ>(addr, data); }

template<typename T> T readarr(u8 *arr, u32 offset)
{
	if constexpr (std::endian::native == std::endian::little) {
		T x;
		std::memcpy(&x, arr + offset, sizeof(T));

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
		std::memcpy(arr + offset, &data, sizeof(T));
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
