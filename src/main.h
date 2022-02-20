#ifndef GBAFLARE_MAIN_H
#define GBAFLARE_MAIN_H

#include <string>

void emulator_init(const std::string &cartridge_filename);
void main_loop();
void emulator_close();

#endif
