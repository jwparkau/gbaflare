#include "memory.h"
#include "cpu.h"

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <regex>

Cartridge cartridge;

u8 bios_data[BIOS_SIZE];
u8 ewram_data[EWRAM_SIZE];
u8 iwram_data[IWRAM_SIZE];
u8 io_data[IO_SIZE];
u8 palette_data[PALETTE_RAM_SIZE];
u8 vram_data[VRAM_SIZE];
u8 oam_data[OAM_SIZE];
u8 cartridge_data[CARTRIDGE_SIZE];
u8 sram_data[SRAM_SIZE];

u32 last_bios_opcode;

u8 *const region_to_data[NUM_REGIONS] = {
	nullptr,
	bios_data,
	ewram_data,
	iwram_data,
	io_data,
	palette_data,
	vram_data,
	oam_data,
	cartridge_data,
	sram_data
};

void request_interrupt(u16 flag)
{
	u16 inter_flag = io_read<u16>(IO_IF);
	inter_flag |= flag;
	io_write<u16>(IO_IF, inter_flag);
}

void load_cartridge_rom()
{
	std::ifstream f(cartridge.filename, std::ios_base::binary);

	f.read((char *)cartridge_data, CARTRIDGE_SIZE);
	auto bytes_read = f.gcount();

	if (bytes_read == 0) {
		throw std::runtime_error("ERROR while reading cartridge file");
	}
	cartridge.size = bytes_read;

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

void determine_save_type()
{
	std::regex r("SRAM_V\\d\\d\\d|FLASH_\\d\\d\\d|FLASH512_V\\d\\d\\d|FLASH1M_V\\d\\d\\d");
	std::cmatch m;

	if (std::regex_search((const char *)cartridge_data, (const char *)cartridge_data+cartridge.size, m, r)) {
		fprintf(stderr, "detected save type -- %s\n", m[0].str().c_str());
		cartridge.save_type_known = true;

		if (m[0].str().starts_with("SRAM")) {
			cartridge.save_type = SAVE_SRAM;
		} else if (m[0].str().starts_with("FLASH_") || m[0].str().starts_with("FLASH512_")) {
			cartridge.save_type = SAVE_FLASH64;
		} else if (m[0].str().starts_with("FLASH1M_")) {
			cartridge.save_type = SAVE_FLASH128;
		}

		return;
	}
}

void load_sram()
{
	std::ifstream f(cartridge.save_file, std::ios_base::binary);

	f.read((char *)sram_data, SRAM_SIZE);
	auto bytes_read = f.gcount();

	fprintf(stderr, "sram save file: read %ld bytes\n", bytes_read);
}

void save_sram()
{
	std::ofstream f(cartridge.save_file, std::ios_base::binary);
	f.write((char *)sram_data, SRAM_SIZE);

	fprintf(stderr, "saved sram to file: %s\n", cartridge.save_file.c_str());
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
		case 0x0E:
		case 0x0F:
			region = MemoryRegion::SRAM;
			offset = addr % SRAM_SIZE;
			break;
		default:
			region = MemoryRegion::UNUSED;
	}

	return offset;
}
bool in_vram_bg(u32 offset) {
	bool bitmap_mode = (io_data[0] & 0x7) >= 3;
	if (bitmap_mode) {
		return offset < 0x14000;
	} else {
		return offset < 0x10000;
	}
}
