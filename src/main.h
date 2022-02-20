#ifndef GBAFLARE_MAIN_H
#define GBAFLARE_MAIN_H

#include <string>

struct Arguments {
	std::string bios_filename;
	std::string cartridge_filename;
};

void emulator_init(Arguments &args);
void main_loop();
void emulator_close();

#endif
