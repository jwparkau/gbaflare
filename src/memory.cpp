#include "memory.h"

#include <stdexcept>
#include <fstream>

u8 bios_data[BIOS_SIZE];
u8 ewram_data[EWRAM_SIZE];
u8 iwram_data[IWRAM_SIZE];
u8 io_data[IO_SIZE];
u8 palette_data[PALETTE_RAM_SIZE];
u8 vram_data[VRAM_SIZE];
u8 oam_data[OAM_SIZE];
u8 cartridge_data[CARTRIDGE_SIZE];

template<typename T> static T read(addr_t addr);
template<typename T> static void write(addr_t addr, T data);


void load_cartridge_rom(const char *filename)
{
	std::ifstream f(filename, std::ios_base::binary);

	f.read((char *)cartridge_data, CARTRIDGE_SIZE);
	std::streamsize bytes_read = f.gcount();

	if (bytes_read == 0) {
		throw std::runtime_error("ERROR while reading cartridge file");
	}

	fprintf(stderr, "cartridge rom: read %ld bytes\n", bytes_read);
}

void load_bios_rom(const char *filename)
{
	std::ifstream f(filename, std::ios_base::binary);

	f.read((char *)bios_data, BIOS_SIZE);
	std::streamsize bytes_read = f.gcount();

	if (bytes_read == 0) {
		throw std::runtime_error("ERROR while reading bios file");
	}

	fprintf(stderr, "bios rom: read %ld bytes\n", bytes_read);
}

void set_initial_memory_state()
{
	write<u16>(IO_KEYINPUT, 0xFFFF);
}

/*
 * Use a software cache later to make things faster
 */
addr_t resolve_memory_address(addr_t addr, MemoryRegion &region)
{
	addr_t offset = 0;
	addr_t l1 = addr >> 24;

	region = MemoryRegion::UNUSED;

	switch (l1) {
		case 0x00:
			if (addr < BIOS_END) {
				region = MemoryRegion::BIOS;
				offset = addr;
			}
			break;
		case 0x02:
			region = MemoryRegion::EWRAM;
			offset = addr % EWRAM_SIZE;
			break;
		case 0x03:
			region = MemoryRegion::IWRAM;
			offset = addr % IWRAM_SIZE;
			break;
		case 0x04:
			if (addr < IO_END) {
				region = MemoryRegion::IO;
				offset = addr & BITMASK(24);
			} else if ((addr & 0xFFFF) == 0x0800) {
				region = MemoryRegion::IO;
				offset = 0x0800;
			}
			break;
		case 0x05:
			region = MemoryRegion::PALETTE_RAM;
			offset = addr % PALETTE_RAM_SIZE;
			break;
		case 0x06:
			region = MemoryRegion::VRAM;
			offset = addr % (VRAM_SIZE + 32_KiB);
			if (offset >= VRAM_SIZE) {
				offset -= 32_KiB;
			}
			break;
		case 0x07:
			region = MemoryRegion::OAM;
			offset = addr % OAM_SIZE;
			break;
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
			region = MemoryRegion::CARTRIDGE;
			offset = addr % CARTRIDGE_SIZE;
			break;
		default:
			region = MemoryRegion::UNUSED;
	}

	return offset;
}

template<typename T>
static T read(addr_t addr)
{
	MemoryRegion region = MemoryRegion::UNUSED;

	std::size_t offset = resolve_memory_address(addr, region);

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
		case MemoryRegion::CARTRIDGE:
			arr = cartridge_data;
			break;
		default:
			return BITMASK(sizeof(T));
	}

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
	MemoryRegion region = MemoryRegion::UNUSED;

	std::size_t offset = resolve_memory_address(addr, region);

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
		case MemoryRegion::CARTRIDGE:
			arr = cartridge_data;
			break;
		default:
			return;
	}

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
