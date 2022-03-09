#include <platform/qt/mainwindow.h>
#include <platform/common/platform.h>
#include <gba/emulator.h>
#include <gba/memory.h>

#include <QApplication>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	w.scale_view();

	fprintf(stderr, "GBAFLare - Gameboy Advance Emulator\n");

	const char **fn;
	for (fn = bios_filenames; *fn; fn++) {
		fprintf(stderr, "trying bios file %s\n", *fn);
		std::ifstream f(*fn);
		if (f.good()) {
			break;
		}
	}
	if (*fn) {
		args.bios_filename = std::string(*fn);
	} else {
		int err = find_bios_file(args.bios_filename);
		if (err) {
			fprintf(stderr, "no bios file\n");
			return EXIT_FAILURE;
		}
	}

	load_bios_rom(args.bios_filename);

	if (argc >= 2) {
		shared.cartridge_filename = std::string(argv[1]);
		emu_cnt.request_open = true;
	}

	emu_cnt.emulator_running = true;
	w.emu_thread->start();

	int err = a.exec();
	return err;
}
