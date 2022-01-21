#ifndef GBAFLARE_MEMORY_H
#define GBAFLARE_MEMORY_H

#include "types.h"

constexpr std::size_t BIOS_SIZE = 16_KiB;
constexpr std::size_t EWRAM_SIZE = 256_KiB;
constexpr std::size_t IWRAM_SIZE = 32_KiB;
constexpr std::size_t IO_SIZE = 1_KiB;
constexpr std::size_t PALETTE_RAM_SIZE = 1_KiB;
constexpr std::size_t VRAM_SIZE = 96_KiB;
constexpr std::size_t OAM_SIZE = 1_KiB;
constexpr std::size_t CARTRIDGE_SIZE = 32_MiB;

constexpr addr_t BIOS_START = 0x0;
constexpr addr_t EWRAM_START = 0x0200'0000;
constexpr addr_t IWRAM_START = 0x0300'0000;
constexpr addr_t IO_START = 0x0400'0000;
constexpr addr_t PALETTE_RAM_START = 0x0500'0000;
constexpr addr_t VRAM_START = 0x0600'0000;
constexpr addr_t OAM_START = 0x0700'0000;
constexpr addr_t CARTRIDGE_START = 0x0800'0000;

constexpr addr_t BIOS_END = BIOS_START + BIOS_SIZE;
constexpr addr_t EWRAM_END = EWRAM_START + EWRAM_SIZE;
constexpr addr_t IWRAM_END = IWRAM_START + IWRAM_SIZE;
constexpr addr_t IO_END = IO_START + IO_SIZE;
constexpr addr_t PALETTE_RAM_END = PALETTE_RAM_START + PALETTE_RAM_SIZE;
constexpr addr_t VRAM_END = VRAM_START + VRAM_SIZE;
constexpr addr_t OAM_END = OAM_START + OAM_SIZE;
constexpr addr_t CARTRIDGE_END = CARTRIDGE_START + CARTRIDGE_SIZE;

enum io_registers : u32 {
	IO_DISPCNT	= 0x0400'0000,
	IO_DISPSTAT	= 0x0400'0004,
	IO_VCOUNT	= 0x0400'0006,
	IO_BG0CNT	= 0x0400'0008,
	IO_BG1CNT	= 0x0400'000A,
	IO_BG2CNT	= 0x0400'000C,
	IO_BG3CNT	= 0x0400'000E,
	IO_BG0HOFS	= 0x0400'0010,
	IO_BG0VOFS	= 0x0400'0012,
	IO_BG1HOFS	= 0x0400'0014,
	IO_BG1VOFS	= 0x0400'0016,
	IO_BG2HOFS	= 0x0400'0018,
	IO_BG2VOFS	= 0x0400'001A,
	IO_BG3HOFS	= 0x0400'001C,
	IO_BG3VOFS	= 0x0400'001E,
	IO_BG2PA	= 0x0400'0020,
	IO_BG2PB	= 0x0400'0022,
	IO_BG2PC	= 0x0400'0024,
	IO_BG2PD	= 0x0400'0026,
	IO_BG2X_L	= 0x0400'0028,
	IO_BG2X_H	= 0x0400'002A,
	IO_BG2Y_L	= 0x0400'002C,
	IO_BG2Y_H	= 0x0400'002E,
	IO_BG3PA	= 0x0400'0030,
	IO_BG3PB	= 0x0400'0032,
	IO_BG3PC	= 0x0400'0034,
	IO_BG3PD	= 0x0400'0036,
	IO_BG3X_L	= 0x0400'0038,
	IO_BG3X_H	= 0x0400'003A,
	IO_BG3Y_L	= 0x0400'003C,
	IO_BG3Y_H	= 0x0400'003E,
	IO_WIN0H	= 0x0400'0040,
	IO_WIN1H	= 0x0400'0042,
	IO_WIN0V	= 0x0400'0044,
	IO_WIN1V	= 0x0400'0046,
	IO_WININ	= 0x0400'0048,
	IO_WINOUT	= 0x0400'004A,
	IO_BLDCNT	= 0x0400'0050,
	IO_BLDALPHA	= 0x0400'0052,
	IO_BLDY		= 0x0400'0054,
	IO_DMA0SAD	= 0x0400'00B0,
	IO_DMA0DAD	= 0x0400'00B4,
	IO_DMA0CNT_L	= 0x0400'00B8,
	IO_DMA0CNT_H	= 0x0400'00BA,
	IO_DMA1CNT_H	= 0x0400'00C6,
	IO_DMA2CNT_H	= 0x0400'00D2,
	IO_DMA3CNT_H	= 0x0400'00DE,
	IO_TM0CNT_L	= 0x0400'0100,
	IO_TM1CNT_L	= 0x0400'0104,
	IO_TM2CNT_L	= 0x0400'0108,
	IO_TM3CNT_L	= 0x0400'010C,
	IO_TM0CNT_H	= 0x0400'0102,
	IO_TM1CNT_H	= 0x0400'0106,
	IO_TM2CNT_H	= 0x0400'010A,
	IO_TM3CNT_H	= 0x0400'010E,
	IO_KEYINPUT	= 0x0400'0130,
	IO_KEYCNT	= 0x0400'0132,
	IO_IE		= 0x0400'0200,
	IO_IF		= 0x0400'0202,
	IO_IME		= 0x0400'0208,
};

enum io_request_flags : u32 {
	IRQ_VBLANK	= 0x1,
	IRQ_HBLANK	= 0x2,
	IRQ_VCOUNTER	= 0x4,
	IRQ_TIMER0	= 0x8,
	IRQ_TIMER1 	= 0x10,
	IRQ_TIMER2 	= 0x20,
	IRQ_TIMER3 	= 0x40,
	IRQ_SERIAL 	= 0x80,
	IRQ_DMA0	= 0x100,
	IRQ_DMA1 	= 0x200,
	IRQ_DMA2 	= 0x400,
	IRQ_DMA3 	= 0x800,
	IRQ_KEYPAD	= 0x1000,
	IRQ_GAMEPAK	= 0x2000
};


enum MemoryRegion {
	BIOS,
	EWRAM,
	IWRAM,
	IO,
	PALETTE_RAM,
	VRAM,
	OAM,
	CARTRIDGE,
	UNUSED
};

extern u8 bios_data[BIOS_SIZE];
extern u8 ewram_data[EWRAM_SIZE];
extern u8 iwram_data[IWRAM_SIZE];
extern u8 io_data[IO_SIZE];
extern u8 palette_data[PALETTE_RAM_SIZE];
extern u8 vram_data[VRAM_SIZE];
extern u8 oam_data[OAM_SIZE];
extern u8 cartridge_data[CARTRIDGE_SIZE];

void request_interrupt(u16 flag);

u32 resolve_memory_address(addr_t addr, MemoryRegion &region);

void load_bios_rom(const char *filename);
void load_cartridge_rom(const char *filename);

void set_initial_memory_state();

namespace Memory {
	u32 read32(addr_t addr);
	u16 read16(addr_t addr);
	u8 read8(addr_t addr);

	void write32(addr_t addr, u32 data);
	void write16(addr_t addr, u16 data);
	void write8(addr_t addr, u8 data);
}

template<typename T> inline T readarr(u8 *arr, u32 offset)
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

template<typename T> inline void writearr(u8 *arr, u32 offset, T data)
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

template<typename T> inline T io_read(addr_t addr)
{
	return readarr<T>(io_data, addr - IO_START);
}

template<typename T> inline void io_write(addr_t addr, T data)
{
	writearr<T>(io_data, addr - IO_START, data);
}

#endif
