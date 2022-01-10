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

enum io_registers {
	IO_DISPCNT	= 0x0400'0000,
	IO_DISPSTAT	= 0x0400'0004,
	IO_VCOUNT	= 0x0400'0006,
	IO_KEYINPUT	= 0x0400'0130,
	IO_IE		= 0x0400'0200,
	IO_IF		= 0x0400'0202,
	IO_IME		= 0x0400'0208,
};

enum io_request_flags {
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

enum io_dispstat_flags {
	LCD_VBLANK	= 0x1,
	LCD_HBLANK	= 0x2,
	LCD_VCOUNTER	= 0x4,
	LCD_VBLANK_IRQ	= 0x8,
	LCD_HBLANK_IRQ	= 0x10,
	LCD_VOUNTER_IRQ	= 0x20
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

addr_t resolve_memory_address(addr_t addr, MemoryRegion &region);

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

template<typename T> inline T readarr(u8 *arr, std::size_t offset)
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

template<typename T> inline void writearr(u8 *arr, std::size_t offset, T data)
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
