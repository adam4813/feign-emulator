#pragma once
#include "Binary.h"

namespace Processor {
    class Z80;
}

namespace Memory {
    struct RTC {

    };

    class MMU {
    public:
        MMU();
        ~MMU();

        // Set the CPU pointer.
        void SetCPU(Processor::Z80* p);

        // Allocate the ROM buffer and optionally copy data into it.
        void AllocateROM(unsigned int size, unsigned char* data = nullptr);

        // Read a byte of memory at address.
        Byte ReadByte(Word& address) {
            if (address < 0x4000) {
                return this->_rom[address];
            }
            else if (address < 0x8000) {
                return this->_rom[((this->rombank - 1) * 0x4000) + address];
            }
            else {
                return this->ram[address - 0x8000];
            }
        }

        // Read a word at address. Computed by add address to (address+1 << 8)
        Word ReadWord(Word& address) {
            if (address < 0x4000) {
                return ((Word)this->_rom[address] + ((Word)this->_rom[address + 1] << 8));
            }
            else if (address < 0x8000) {
                return ((Word)this->_rom[((this->rombank - 1) * 0x4000) + address] + ((Word)this->_rom[((this->rombank - 1) * 0x4000) + address + 1] << 8));
            }
            else {
                return ((Word)this->ram[address - 0x8000] + ((Word)this->ram[address - 0x8000 + 1] << 8));
            }
        }

        // Read a byte of memory at address.
        Byte ReadByte(Word&& address) {
            if (address < 0x4000) {
                return this->_rom[address];
            }
            else if (address < 0x8000) {
                return this->_rom[((this->rombank - 1) * 0x4000) + address];
            }
            else {
                if ((address > 0xE000) && (address < 0xFE00)) {
                    return this->ram[address - 0x9000];
                }
                return this->ram[address - 0x8000];
            }
        }

        // Read a word at address. Computed by add address to (address+1 << 8)
        Word ReadWord(Word&& address) {
            if (address < 0x4000) {
                return ((Word)this->_rom[address] + ((Word)this->_rom[address + 1] << 8));
            }
            else if (address < 0x8000) {
                return ((Word)this->_rom[((this->rombank - 1) * 0x4000) + address] + ((Word)this->_rom[((this->rombank - 1) * 0x4000) + address + 1] << 8));
            }
            else {
                return ((Word)this->ram[address - 0x8000] + ((Word)this->ram[address - 0x8000 + 1] << 8));
            }
        }

        // Write a byte at address.
        void WriteByte(const Word& address, const Byte& val);

        // Write a word at address. The low byte is written at address, and the high byte is written at address+1
        void WriteWord(const Word& address, const Word& val);

        void SetCatridgeType(Byte type);
    private:
        Byte _bios[256];
        Byte* _rom; // Swappable
        //Byte* mem;
        Byte ram[0x8000];

        bool _inbios;

        unsigned int ROMSize;

        Processor::Z80* cpu;

        Word romOffset;
        Word ramOffset;

        Byte cartType;

        int rombank; // Selected ROM bank
        int rambank; // Selected RAM bank
        bool ramOn; // RAM enabled
        Byte mode; // ROM/RAM expansion mode
    };
}