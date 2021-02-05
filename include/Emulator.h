#pragma once

class Emulator {
public:
	Emulator(void) : InitialPC(0) {
	}

	~Emulator(void) {
	}

	// TODO: Implement paged memory access
	inline char ReadMemory(unsigned int Address) {
		return Memory[Address];
	}

	// TODO: Implement paged memory access
	inline void WriteMemory(unsigned int Address, char Value) {
		Memory[Address] = Value;
	}

protected:
	int counter;
	int intPeriod;
	int PC;
	const int InitialPC;

	char* Memory;

	int* Cycles;
	bool ExitRequired;

};

