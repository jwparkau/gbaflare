#include <gba/eeprom.h>
#include <gba/memory.h>

#include <iostream>
#include <fstream>

Eeprom eeprom;

#define TO(x) eeprom_state = x

Eeprom::Eeprom()
{
	std::memset(eeprom_memory, 0xFF, MAX_EEPROM_SIZE);
}

void Eeprom::reset()
{
	eeprom_mask = 0xFFFF'FFFF;
	eeprom_state = 0;
	address_bits_left = 0;
	data_bits_left = 0;
	read_bits_left = 0;
	read_op = 0;
	address = 0;
	bytes = 0;
	std::memset(eeprom_memory, 0xFF, MAX_EEPROM_SIZE);
}

u8 Eeprom::read()
{
	bool x = 1;

	if (read_bits_left > 64) {
		;
	} else if (read_bits_left > 0) {
		bool high = read_bits_left > 32;

		u32 offset = high ? address * 8 + 4 : address * 8;
		u32 n = high ? read_bits_left - 1 : read_bits_left - 32 - 1;

		x = readarr<u32>(eeprom_memory, offset) & BIT(n);
	} else {
		;
	}

	if (read_bits_left > 0) {
		read_bits_left--;
	}

	return x;
}

void Eeprom::write(u8 data)
{
	switch (eeprom_state) {
		case EEPROM_READY:
			if (data) {
				TO(EEPROM_1);
			}
			break;
		case EEPROM_1:
			if (cartridge.save_type == SAVE_EEPROM_UNKNOWN) {
				if (dma.transfers[3].cnt_l == 17) {
					cartridge.save_type = SAVE_EEPROM64;
				} else {
					cartridge.save_type = SAVE_EEPROM4;
				}
			}

			if (cartridge.save_type == SAVE_EEPROM64) {
				address_bits_left = 14;
			} else {
				address_bits_left = 6;
			}

			if (data) {
				read_op = true;
			} else {
				read_op = false;
			}

			address = 0;
			TO(EEPROM_ADDRESS);
			break;
		case EEPROM_ADDRESS:
			address = address << 1 | data;
			address_bits_left--;
			if (address_bits_left == 0) {
				if (read_op) {
					TO(EEPROM_END);
				} else {
					data_bits_left = 64;
					bytes = 0;
					TO(EEPROM_DATA);
				}
			}
			break;
		case EEPROM_DATA:
			bytes = bytes << 1 | data;
			data_bits_left--;
			if (data_bits_left == 0) {
				u32 offset = address * 8;
				writearr<u32>(eeprom_memory, offset, bytes);
				writearr<u32>(eeprom_memory, offset+4, bytes >> 32);

				TO(EEPROM_END);
			}
			break;
		case EEPROM_END:
			if (read_op) {
				read_bits_left = 68;
			}
			TO(EEPROM_READY);
	}
}

#undef TO

void load_eeprom()
{
	std::ifstream f(cartridge.save_file, std::ios_base::binary);

	f.read((char *)eeprom.eeprom_memory, MAX_EEPROM_SIZE);
	auto bytes_read = f.gcount();

	fprintf(stderr, "eeprom save file: read %ld bytes\n", bytes_read);
}

void save_eeprom()
{
	std::ofstream f(cartridge.save_file, std::ios_base::binary);
	f.write((char *)eeprom.eeprom_memory, MAX_EEPROM_SIZE);

	fprintf(stderr, "saved eeprom to file: %s\n", cartridge.save_file.c_str());
}

