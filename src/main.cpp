
#include <iostream>
#include "../include/GB.h"

int main(int argc, const char* argv[]) {
	GBoy gbemu;
	gbemu.LoadROMImage("sml.gb");

	while (1) {
		gbemu.Update(0);
	}

	system("Pause");
	return 0;
}