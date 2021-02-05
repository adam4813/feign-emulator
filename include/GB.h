#pragma once

#include "Video.h"
#include "Memory.h"
#include "CPU.h"
#include "Cartridge.h"

// Main memory and video memory are the same size at 8k
#define MEMORY_SIZE 8192

#define NUM_REGISTERS 256

#include <chrono>

using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::chrono::high_resolution_clock;
using std::chrono::duration;

class GBoy {
public:
	GBoy() {
	}

	~GBoy() { }

	// Load a ROM image from file.
	void LoadROMImage(std::string fname) {
		Cartridge cart;
		cart.LoadFromFile(fname);
		std::cout << "Loaded ROM title: " << cart.GetTitle() << std::endl;
		this->MainMemory.AllocateROM(cart.GetSize(), cart.GetBuffer());
		this->MainMemory.SetCatridgeType(cart.GetType());

		/*if (cart.LoadFromFile(fname)) {
			std::cout << "Loaded ROM title: " << cart.GetTitle() << std::endl;
			this->MainMemory.AllocateROM(cart.GetSize(), cart.GetBuffer());
			this->MainMemory.SetCatridgeType(cart.GetType());
		}*/

		// Set the initial PC to be after BIOS.
		this->MainCPU.SetMMU(&this->MainMemory);
		this->MainMemory.SetCPU(&this->MainCPU);
		this->MainVideo.SetCPU(&this->MainCPU);
		this->MainVideo.SetRAM(&this->MainMemory);
		this->MainCPU.SetPC(0x100);
	}

	bool Update(unsigned int clocks) {
		bool exit = false;
		exit =  this->MainCPU.DoNextOp();
		this->MainVideo.Step();
        this->MainCPU.DoInterrupts();

		return exit;
	}
private:
	Memory::MMU MainMemory;

	Processor::Z80 MainCPU;

	Video::DMG MainVideo;

	high_resolution_clock::time_point start;
	high_resolution_clock::time_point end;
};

