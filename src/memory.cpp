#include "memory.h"
#include "timer.h"

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

static u8 *const region_to_data[9] = {
	bios_data,
	ewram_data,
	iwram_data,
	io_data,
	palette_data,
	vram_data,
	oam_data,
	cartridge_data,
	nullptr
};

template<typename T> static T read(addr_t addr);
template<typename T> static void write(addr_t addr, T data);
template<typename T> static T mmio_read(addr_t addr);
template<typename T> static void mmio_write(addr_t addr, T data);

void request_interrupt(u16 flag)
{
	u16 inter_flag = io_read<u16>(IO_IF);
	inter_flag |= flag;
	io_write<u16>(IO_IF, inter_flag);
}

void load_cartridge_rom(const char *filename)
{
	std::ifstream f(filename, std::ios_base::binary);

	f.read((char *)cartridge_data, CARTRIDGE_SIZE);
	auto bytes_read = f.gcount();

	if (bytes_read == 0) {
		throw std::runtime_error("ERROR while reading cartridge file");
	}

	fprintf(stderr, "cartridge rom: read %ld bytes\n", bytes_read);
}

void load_bios_rom(const char *filename)
{
	std::ifstream f(filename, std::ios_base::binary);

	f.read((char *)bios_data, BIOS_SIZE);
	auto bytes_read = f.gcount();

	if (bytes_read == 0) {
		throw std::runtime_error("ERROR while reading bios file");
	}

	fprintf(stderr, "bios rom: read %ld bytes\n", bytes_read);
}

void set_initial_memory_state()
{
	io_write<u16>(IO_KEYINPUT, 0xFFFF);
}

/*
 * Use a software cache later to make things faster
 */
u32 resolve_memory_address(addr_t addr, MemoryRegion &region)
{
	region = MemoryRegion::UNUSED;
	u32 offset = 0;

	switch (addr >> 24) {
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
	if constexpr (sizeof(T) == sizeof(u8)) {
		if (addr == 0x0E000000) {
			return 0x62;
		}
		if (addr == 0x0E000001) {
			return 0x13;
		}
	}

	MemoryRegion region = MemoryRegion::UNUSED;

	u32 offset = resolve_memory_address(addr, region);

	u8 *arr = nullptr;

	arr = region_to_data[region];

	if (!arr) {
		return BITMASK(sizeof(T));
	}

	if (region == MemoryRegion::IO) {
		return mmio_read<T>(addr);
	}

	return readarr<T>(arr, offset);
}

template<typename T>
static void write(addr_t addr, T data)
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
		mmio_write<T>(addr, data);
		return;
	}

	writearr<T>(arr, offset, data);
}

template<typename T>
static T mmio_read(addr_t addr)
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

	return x;
}

template<typename T>
static void mmio_write(addr_t addr, T data)
{
	u32 x = data;

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
				timer.on_write(addr + i, new_value);
				break;
		}

		io_data[offset] = new_value;
		x >>= 8;
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
