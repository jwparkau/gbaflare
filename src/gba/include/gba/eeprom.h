#ifndef GBAFLARE_EEPROM_H
#define GBAFLARE_EEPROM_H

#include <common/types.h>

#define MAX_EEPROM_SIZE 8_KiB

enum eeprom_state_t {
	EEPROM_READY,
	EEPROM_1,
	EEPROM_ADDRESS,
	EEPROM_DATA,
	EEPROM_END
};

void load_eeprom();
void save_eeprom();

struct Eeprom {
	u32 eeprom_mask = 0xFFFF'FFFF;
	int eeprom_state{};
	int address_bits_left{};
	int data_bits_left{};
	int read_bits_left{};
	bool read_op{};
	u16 address{};
	u64 bytes{};
	u8 eeprom_memory[MAX_EEPROM_SIZE]{};

	Eeprom();
	void reset();
	u8 read();
	void write(u8 data);
};

extern Eeprom eeprom;

inline u8 eeprom_read()
{
	return eeprom.read();
}

inline void eeprom_write(u8 data)
{
	eeprom.write(data & 1);
}


#endif
