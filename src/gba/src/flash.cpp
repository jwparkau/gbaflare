#include <gba/flash.h>
#include <gba/memory.h>

#include <iostream>
#include <fstream>

Flash flash;

Flash::Flash()
{
	std::memset(flash_memory, 0xFF, MAX_FLASH_SIZE);
}

void Flash::erase_page(int page)
{
	u8 *p = flash_memory;
	if (cartridge.save_type == SAVE_FLASH128 && flash_bank) {
		p += 64_KiB;
	}
	for (u32 i = page << 12, j = 0; j < 4_KiB; i++, j++) {
		p[i] = 0xFF;
	}
}

void Flash::erase_all()
{
	u8 *p = flash_memory;
	if (cartridge.save_type == SAVE_FLASH128 && flash_bank) {
		p += 64_KiB;
	}
	for (u32 i = 0; i < 64_KiB; i++) {
		p[i] = 0xFF;
	}
}

void Flash::reset()
{
	flash_state = FLASH_READY;
	id_mode = false;
	flash_bank = false;
	std::memset(flash_memory, 0xFF, MAX_FLASH_SIZE);
}

void load_flash()
{
	std::ifstream f(cartridge.save_file, std::ios_base::binary);

	f.read((char *)flash.flash_memory, MAX_FLASH_SIZE);
	auto bytes_read = f.gcount();

	fprintf(stderr, "flash save file: read %ld bytes\n", bytes_read);
}

void save_flash()
{
	std::ofstream f(cartridge.save_file, std::ios_base::binary);
	f.write((char *)flash.flash_memory, MAX_FLASH_SIZE);

	fprintf(stderr, "saved flash to file: %s\n", cartridge.save_file.c_str());
}

