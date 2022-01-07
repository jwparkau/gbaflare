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

enum class MemoryRegion {
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

MemoryRegion get_memory_region(addr_t addr, addr_t &base_addr);

void load_bios_rom(const char *filename);
void load_cartridge_rom(const char *filename);

namespace Memory {
	u32 read32(addr_t addr);
	u16 read16(addr_t addr);
	u8 read8(addr_t addr);

	void write32(addr_t addr, u32 data);
	void write16(addr_t addr, u16 data);
	void write8(addr_t addr, u8 data);
}


#endif
