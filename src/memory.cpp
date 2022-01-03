#include "memory.h"

u8 bios_data[BIOS_SIZE];
u8 ewram_data[EWRAM_SIZE];
u8 iwram_data[IWRAM_SIZE];
u8 io_data[IO_SIZE];
u8 palette_data[PALETTE_RAM_SIZE];
u8 vram_data[VRAM_SIZE];
u8 oam_data[OAM_SIZE];

/*
 * Use a software cache later to make things faster
 */
MemoryRegion get_memory_region(addr_t addr, addr_t &base_addr)
{
	MemoryRegion region;
	if (BIOS_START <= addr && addr < BIOS_END) {
		base_addr = BIOS_START;
		region = MemoryRegion::BIOS;
	} else if (EWRAM_START <= addr && addr < EWRAM_END) {
		base_addr = EWRAM_START;
		region = MemoryRegion::EWRAM;
	} else if (IWRAM_START <= addr && addr < IWRAM_END) {
		base_addr = IWRAM_START;
		region = MemoryRegion::IWRAM;
	} else if (IO_START <= addr && addr < IO_END) {
		base_addr = IO_START;
		region = MemoryRegion::IO;
	} else if (PALETTE_RAM_START <= addr && addr < PALETTE_RAM_END) {
		base_addr = PALETTE_RAM_START;
		region = MemoryRegion::PALETTE_RAM;
	} else if (VRAM_START <= addr && addr < VRAM_END) {
		base_addr = VRAM_START;
		region = MemoryRegion::VRAM;
	} else if (OAM_START <= addr && addr < OAM_END) {
		base_addr = OAM_START;
		region = MemoryRegion::OAM;
	} else {
		region = MemoryRegion::UNUSED;
	}

	return region;
}

template<typename T>
static u32 read(addr_t addr)
{
	addr_t base_addr = 0;
	MemoryRegion region = get_memory_region(addr, base_addr);

	u8 *arr = nullptr;

	switch (region) {
		case MemoryRegion::BIOS:
			arr = bios_data;
			break;
		case MemoryRegion::EWRAM:
			arr = ewram_data;
			break;
		case MemoryRegion::IWRAM:
			arr = iwram_data;
			break;
		case MemoryRegion::IO:
			arr = io_data;
			break;
		case MemoryRegion::PALETTE_RAM:
			arr = palette_data;
			break;
		case MemoryRegion::VRAM:
			arr = vram_data;
			break;
		case MemoryRegion::OAM:
			arr = oam_data;
			break;
		default:
			return 0xFFFF'FFFF;
	}

	std::size_t offset = addr - base_addr;

	if constexpr (sizeof(T) == sizeof(u32)) {
		return readarr32(arr, offset);
	} else if constexpr (sizeof(T) == sizeof(u16)) {
		return readarr16(arr, offset);
	} else {
		return readarr8(arr, offset);
	}
}

template<typename T>
static void write(addr_t addr, T data)
{
	addr_t base_addr = 0;
	MemoryRegion region = get_memory_region(addr, base_addr);

	u8 *arr = nullptr;

	switch (region) {
		case MemoryRegion::BIOS:
			return;
		case MemoryRegion::EWRAM:
			arr = ewram_data;
			break;
		case MemoryRegion::IWRAM:
			arr = iwram_data;
			break;
		case MemoryRegion::IO:
			arr = io_data;
			break;
		case MemoryRegion::PALETTE_RAM:
			arr = palette_data;
			break;
		case MemoryRegion::VRAM:
			arr = vram_data;
			break;
		case MemoryRegion::OAM:
			arr = oam_data;
			break;
		default:
			return;
	}

	std::size_t offset = addr - base_addr;

	if constexpr (sizeof(T) == sizeof(u32)) {
		writearr32(arr, offset, data);
	} else if constexpr (sizeof(T) == sizeof(u16)) {
		writearr16(arr, offset, data);
	} else {
		writearr8(arr, offset, data);
	}
}

u32 Memory::read32(addr_t addr)
{
	return read<u32>(addr);
}

u16 Memory::read16(addr_t addr)
{
	return read<u16>(addr);
}

u8 Memory::read8(addr_t addr)
{
	return read<u8>(addr);
}

void Memory::write32(addr_t addr, u32 data)
{
	write<u32>(addr, data);
}

void Memory::write16(addr_t addr, u16 data)
{
	write<u16>(addr, data);
}

void Memory::write8(addr_t addr, u8 data)
{
	write<u8>(addr, data);
}
