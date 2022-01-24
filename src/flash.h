#ifndef GBAFLARE_FLASH_H
#define GBAFLARE_FLASH_H

#include "types.h"

enum flash_state {
	FLASH_READY,
	FLASH_CMD1,
	FLASH_CMD2,
	FLASH_ERASE,
	FLASH_ERASE1,
	FLASH_ERASE2,
	FLASH_WRITE,
	FLASH_SET_BANK
};

#define MAX_FLASH_SIZE 128_KiB

struct Flash {
	int flash_state{};
	bool id_mode{};
	u8 flash_memory[MAX_FLASH_SIZE]{};
	bool flash_bank{};

	Flash();
	void erase_page(int page);
	void erase_all();
};

extern Flash flash;

void load_flash();
void save_flash();

template<typename T, int flash_size> T flash_read(addr_t addr)
{
	if (flash.id_mode) {
		if (addr == 0x0E00'0000) {
			if constexpr (flash_size == 64) {
				return 0x32;
			} else {
				return 0x62;
			}
		} else if (addr == 0x0E00'0001) {
			if constexpr (flash_size == 64) {
				return 0x1B;
			} else {
				return 0x13;
			}
		}
	}

	u8 *p = flash.flash_memory;

	if constexpr (flash_size == 128) {
		if (flash.flash_bank) {
			p += 64_KiB;
		}
	}

	u8 x = p[addr % 64_KiB];

	if constexpr (sizeof(T) == sizeof(u8)) {
		return x;
	} else if constexpr (sizeof(T) == sizeof(u16)) {
		return x * 0x101;
	} else {
		return x * 0x1010101;
	}
}

#define A1 0xE005555
#define A2 0xE002AAA

#define TO(x) flash.flash_state = x

template<typename T, int flash_size> void flash_write(addr_t addr, T data)
{
	switch (flash.flash_state) {
		case FLASH_ERASE:
		case FLASH_ERASE1:
		case FLASH_ERASE2:
		case FLASH_WRITE:
			if (addr == A1 && data == 0xF0) {
				TO(FLASH_READY);
				return;
			}
			break;
	}

	u8 *p = flash.flash_memory;

	switch (flash.flash_state) {
		case FLASH_READY:
			if (addr == A1 && data == 0xAA) {
				TO(FLASH_CMD1);
			}
			break;
		case FLASH_CMD1:
			if (addr == A2 && data == 0x55) {
				TO(FLASH_CMD2);
			}
			break;
		case FLASH_CMD2:
			if (addr == A1) {
				if (data == 0x90) {
					flash.id_mode = true;
					TO(FLASH_READY);
				} else if (data == 0xF0) {
					flash.id_mode = false;
					TO(FLASH_READY);
				} else if (data == 0x80) {
					TO(FLASH_ERASE);
				} else if (data == 0xA0) {
					TO(FLASH_WRITE);
				} else if (data == 0xB0) {
					TO(FLASH_SET_BANK);
				}
			}
			break;
		case FLASH_ERASE:
			if (addr == A1 && data == 0xAA) {
				TO(FLASH_ERASE1);
			}
			break;
		case FLASH_ERASE1:
			if (addr == A2 && data == 0x55) {
				TO(FLASH_ERASE2);
			}
			break;
		case FLASH_ERASE2:
			if (addr == A1 && data == 0x10) {
				flash.erase_all();
			} else if ((addr & 0xFFFF0FFF) == 0x0E000000 && data == 0x30) {
				flash.erase_page(addr >> 12 & BITMASK(4));
			}
			TO(FLASH_READY);
			break;
		case FLASH_WRITE:
			if constexpr (flash_size == 128) {
				if (flash.flash_bank) {
					p += 64_KiB;
				}
			}
			p[addr % 64_KiB] = data;
			TO(FLASH_READY);
			break;
		case FLASH_SET_BANK:
			if (addr == 0x0E00'0000) {
				flash.flash_bank = data;
			}
			TO(FLASH_READY);
			break;
	}
}
#undef A1
#undef A2
#undef TO

#endif
