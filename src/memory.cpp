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

u8 *const region_to_data_write[NUM_REGIONS] = {
	nullptr,
	nullptr,
	ewram_data,
	iwram_data,
	io_data,
	palette_data,
	vram_data,
	oam_data,
	nullptr,
	sram_data
};

const int addr_to_region[16] = {
	MemoryRegion::BIOS,
	MemoryRegion::UNUSED,
	MemoryRegion::EWRAM,
	MemoryRegion::IWRAM,
	MemoryRegion::IO,
	MemoryRegion::PALETTE_RAM,
	MemoryRegion::VRAM,
	MemoryRegion::OAM,
	MemoryRegion::CARTRIDGE,
	MemoryRegion::CARTRIDGE,
	MemoryRegion::CARTRIDGE,
	MemoryRegion::CARTRIDGE,
	MemoryRegion::CARTRIDGE,
	MemoryRegion::CARTRIDGE,
	MemoryRegion::SRAM,
	MemoryRegion::SRAM
};

const u32 region_to_offset_mask[NUM_REGIONS] = {
	0,
	0x3FFF,
	0x3FFFF,
	0x7FFF,
	0x3FF,
	0x3FF,
	0x1FFFF,
	0x3FF,
	0x1FFFFFF,
	0xFFFF
};

void request_interrupt(u16 flag)
{
	u16 inter_flag = io_read<u16>(IO_IF);
	inter_flag |= flag;
	io_write<u16>(IO_IF, inter_flag);
}

void set_initial_memory_state()
{
	for (u32 i = 0; i < CARTRIDGE_SIZE; i += 2) {
		writearr<u16>(cartridge_data, i, i / 2 & 0xFFFF);
	}

	for (u32 i = 0; i < SRAM_SIZE; i++) {
		sram_data[i] = 0xFF;
	}

	io_write<u16>(IO_KEYINPUT, 0xFFFF);
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
	std::regex r("SRAM_V|FLASH_|FLASH512_V|FLASH1M_V");
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

bool in_vram_bg(u32 offset) {
	bool bitmap_mode = (io_data[0] & 0x7) >= 3;
	if (bitmap_mode) {
		return offset < 0x14000;
	} else {
		return offset < 0x10000;
	}
}
