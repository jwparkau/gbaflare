#ifndef GBAFLARE_MEMORY_H
#define GBAFLARE_MEMORY_H

#include "types.h"
#include "timer.h"
#include "ppu.h"
#include "dma.h"
#include "cpu.h"
#include "flash.h"

#include <string>

constexpr std::size_t BIOS_SIZE = 16_KiB;
constexpr std::size_t EWRAM_SIZE = 256_KiB;
constexpr std::size_t IWRAM_SIZE = 32_KiB;
constexpr std::size_t IO_SIZE = 1_KiB;
constexpr std::size_t PALETTE_RAM_SIZE = 1_KiB;
constexpr std::size_t VRAM_SIZE = 96_KiB;
constexpr std::size_t OAM_SIZE = 1_KiB;
constexpr std::size_t CARTRIDGE_SIZE = 32_MiB;
constexpr std::size_t SRAM_SIZE = 64_KiB;

constexpr addr_t BIOS_START = 0x0;
constexpr addr_t EWRAM_START = 0x0200'0000;
constexpr addr_t IWRAM_START = 0x0300'0000;
constexpr addr_t IO_START = 0x0400'0000;
constexpr addr_t PALETTE_RAM_START = 0x0500'0000;
constexpr addr_t VRAM_START = 0x0600'0000;
constexpr addr_t OAM_START = 0x0700'0000;
constexpr addr_t CARTRIDGE_START = 0x0800'0000;
constexpr addr_t SRAM_START = 0x0E00'0000;

constexpr addr_t BIOS_END = BIOS_START + BIOS_SIZE;
constexpr addr_t EWRAM_END = EWRAM_START + EWRAM_SIZE;
constexpr addr_t IWRAM_END = IWRAM_START + IWRAM_SIZE;
constexpr addr_t IO_END = IO_START + IO_SIZE;
constexpr addr_t PALETTE_RAM_END = PALETTE_RAM_START + PALETTE_RAM_SIZE;
constexpr addr_t VRAM_END = VRAM_START + VRAM_SIZE;
constexpr addr_t OAM_END = OAM_START + OAM_SIZE;
constexpr addr_t CARTRIDGE_END = CARTRIDGE_START + CARTRIDGE_SIZE;
constexpr addr_t SRAM_END = SRAM_START + SRAM_SIZE;

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
	IO_MOSAIC	= 0x0400'004C,
	IO_BLDCNT	= 0x0400'0050,
	IO_BLDALPHA	= 0x0400'0052,
	IO_BLDY		= 0x0400'0054,
	IO_DMA0SAD	= 0x0400'00B0,
	IO_DMA0DAD	= 0x0400'00B4,
	IO_DMA0CNT_L	= 0x0400'00B8,
	IO_DMA1CNT_L	= 0x0400'00C4,
	IO_DMA2CNT_L	= 0x0400'00D0,
	IO_DMA3CNT_L	= 0x0400'00DC,
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
	UNUSED,
	BIOS,
	EWRAM,
	IWRAM,
	IO,
	PALETTE_RAM,
	VRAM,
	OAM,
	CARTRIDGE,
	SRAM,
	NUM_REGIONS
};

enum MemoryAccessFrom {
	FROM_CPU,
	FROM_DMA,
	ALLOW_ALL
};

enum SaveType {
	SAVE_NONE,
	SAVE_FLASH64,
	SAVE_FLASH128,
	SAVE_SRAM
};

struct Cartridge {
	bool save_type_known{};
	int save_type{};
	std::size_t size{};
	std::string filename;
	std::string save_file;
};

extern Cartridge cartridge;

extern u8 bios_data[BIOS_SIZE];
extern u8 ewram_data[EWRAM_SIZE];
extern u8 iwram_data[IWRAM_SIZE];
extern u8 io_data[IO_SIZE];
extern u8 palette_data[PALETTE_RAM_SIZE];
extern u8 vram_data[VRAM_SIZE];
extern u8 oam_data[OAM_SIZE];
extern u8 cartridge_data[CARTRIDGE_SIZE];
extern u8 sram_data[SRAM_SIZE];

extern u8 *const region_to_data[NUM_REGIONS];
extern u32 last_bios_opcode;

void request_interrupt(u16 flag);
u32 resolve_memory_address(addr_t addr, MemoryRegion &region);
void load_bios_rom(const char *filename);
void load_cartridge_rom();
void determine_save_type();
void load_sram();
void save_sram();
void set_initial_memory_state();
bool in_vram_bg(addr_t addr);

template<typename T> T io_read(addr_t addr)
{
	return readarr<T>(io_data, addr - IO_START);
}

template<typename T> void io_write(addr_t addr, T data)
{
	writearr<T>(io_data, addr - IO_START, data);
}

template<typename T, int type> T sram_read(addr_t addr)
{
	u32 x = readarr<u8>(sram_data, addr % SRAM_SIZE);

	if constexpr (sizeof(T) == sizeof(u8)) {
		return x;
	} else if constexpr (sizeof(T) == sizeof(u16)) {
		return x * 0x101;
	} else {
		return x * 0x1010101;
	}
}

template<typename T, int type> void sram_write(addr_t addr, T data)
{
	writearr<u8>(sram_data, addr % SRAM_SIZE, data & BITMASK(8));
}


template<typename T, int type> T read(addr_t addr)
{
	MemoryRegion region = MemoryRegion::UNUSED;

	u32 offset = resolve_memory_address(addr, region);

	u8 *arr = nullptr;

	arr = region_to_data[region];

	if (region == MemoryRegion::SRAM) {
		if (cartridge.save_type == SAVE_SRAM) {
			return sram_read<T, type>(addr);
		} else if (cartridge.save_type == SAVE_FLASH64) {
			return flash_read<T, 64>(addr);
		} else if (cartridge.save_type == SAVE_FLASH128) {
			return flash_read<T, 128>(addr);
		}
	}

	if (region == MemoryRegion::IO) {
		return mmio_read<T, type>(addr);
	}

	if constexpr (type == FROM_CPU) {
		if (region == MemoryRegion::BIOS) {
			if (cpu.pc - 8 >= BIOS_END) {
				if constexpr (sizeof(T) == sizeof(u8)) {
					return last_bios_opcode >> (addr % 4 * 8);
				} else {
					return last_bios_opcode;
				}
			}
		} else if (addr < 0x0200'0000 || addr >= 0x1000'0000) {
			u32 x = read<u32, ALLOW_ALL>(cpu.pc);
			if constexpr (sizeof(T) == sizeof(u8)) {
				return x >> (addr % 4 * 8);
			} else {
				return x;
			}
		}
	}

	if constexpr (type == FROM_DMA) {
		if (addr < 0x0200'0000 || addr >= 0x1000'0000) {
			return dma.last_value[dma.channel];
		}
	}

	if (!arr) {
		return BITMASK(sizeof(T));
	}

	T x = readarr<T>(arr, offset);

	if constexpr (type == FROM_DMA) {
		dma.last_value[dma.channel] = x;
	}

	return x;
}

template<typename T, int type> void write(addr_t addr, T data)
{
	MemoryRegion region = MemoryRegion::UNUSED;

	u32 offset = resolve_memory_address(addr, region);

	switch (region) {
		case MemoryRegion::BIOS:
		case MemoryRegion::CARTRIDGE:
			return;
		default:
			break;
	}

	u8 *arr = region_to_data[region];

	if (!arr) {
		return;
	}

	if (region == MemoryRegion::IO) {
		mmio_write<T, type>(addr, data);
		return;
	}

	if (region == MemoryRegion::SRAM) {
		if (cartridge.save_type == SAVE_SRAM) {
			sram_write<T, type>(addr, data);
		} else if (cartridge.save_type == SAVE_FLASH64) {
			flash_write<T, 64>(addr, data);
		} else if (cartridge.save_type == SAVE_FLASH128) {
			flash_write<T, 128>(addr, data);
		}
		return;
	}

	if constexpr (sizeof(T) == sizeof(u8)) {
		if (region == MemoryRegion::OAM)
			return;

		if (region == MemoryRegion::VRAM) {
			if (!in_vram_bg(offset)) {
				return;
			} else {
				writearr<u16>(arr, align(offset, 2), data * 0x101);
				return;
			}
		}

		if (region == MemoryRegion::PALETTE_RAM) {
			writearr<u16>(arr, align(offset, 2), data * 0x101);
			return;
		}
	}

	writearr<T>(arr, offset, data);
}

template<typename T, int type> T mmio_read(addr_t addr)
{
	u32 x = 0;

	for (std::size_t i = 0; i < sizeof(T); i++) {
		u32 offset = addr - IO_START + i;

		u32 value = io_data[offset];

		switch (addr + i) {
			case IO_TM0CNT_L:
			case IO_TM0CNT_L+1:
			case IO_TM1CNT_L:
			case IO_TM1CNT_L+1:
			case IO_TM2CNT_L:
			case IO_TM2CNT_L+1:
			case IO_TM3CNT_L:
			case IO_TM3CNT_L+1:
				value = timer.on_read(addr + i);
		}

		x |= value << (i * 8);
	}

	if constexpr (type == FROM_DMA) {
		dma.last_value[dma.channel] = x;
	}

	return x;
}

template<typename T, int type> void mmio_write(addr_t addr, T data)
{
	u32 x = data;

	bool update_affine_ref = false;

	for (std::size_t i = 0; i < sizeof(T); i++) {

		u8 to_write = x & BITMASK(8);
		u8 mask;

		switch (addr + i) {
			case IO_KEYINPUT:
			case IO_KEYINPUT + 1:
			case IO_VCOUNT:
				mask = 0;
				break;
			case IO_DISPSTAT:
				mask = 0xF8;
				break;
			case IO_IF:
			case IO_IF + 1:
				mask = to_write;
				to_write = 0;
				break;
			default:
				mask = 0xFF;
		}

		u32 offset = addr - IO_START + i;

		u8 old_value = io_data[offset];
		u8 new_value = (old_value & ~mask) | (to_write & mask);

		switch (addr + i) {
			case IO_TM0CNT_H:
			case IO_TM1CNT_H:
			case IO_TM2CNT_H:
			case IO_TM3CNT_H:
				timer.on_write(addr + i, old_value, new_value);
				break;
			case IO_DMA0CNT_H + 1:
			case IO_DMA1CNT_H + 1:
			case IO_DMA2CNT_H + 1:
			case IO_DMA3CNT_H + 1:
				dma.on_write(addr + i, old_value, new_value);
				break;
		}

		if ((IO_BG2X_L <= addr + i && addr + 1 < IO_BG2Y_H + 2) || (IO_BG3X_L <= addr + i && addr + 1 < IO_BG3Y_H + 2)) {
			update_affine_ref = true;
		}

		io_data[offset] = new_value;
		x >>= 8;
	}

	if (update_affine_ref) {
		ppu.copy_affine_ref();
	}
}

#endif
