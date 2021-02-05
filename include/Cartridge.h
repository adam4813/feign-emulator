#pragma once

#include <string>
#include <fstream>

class Cartridge {
public:
	Cartridge() : buffer(nullptr), fsize(0), badHeaderChecksum(false) { }
	~Cartridge() {
		if (this->buffer != nullptr) {
			delete[] this->buffer;
		}
	}

	// Load a ROM image from file. Return true on successful load and header checksum pass.
	bool LoadFromFile(const std::string& fname) {
		std::ifstream file(fname, std::ios::binary);

		// Move to the end of the file to get the size.
		file.seekg(0, std::ios::end);

		// Save the size.
		this->fsize = (unsigned int)file.tellg();

		// Move to the beginning of the file and copy it to the internal buffer.
		file.seekg(0);

		if (this->fsize > 0) {
			this->buffer = new Byte[this->fsize];

			for (unsigned int i = 0; i < this->fsize; ++i) {
				this->buffer[i] = file.get();
			}

			// Make sure the ROM image is at least as large as the header before trying to parse it.
			if (this->fsize > 0x014F) {
				ParseHeader();
			}
			else {
				return false;
			}

			if (this->badHeaderChecksum) {
				return false;
			}

			return true;
		}

		return false;
	}

	// Parse the header information from the image.
	void ParseHeader() {
		memcpy(this->ninLogo, &this->buffer[0x0104], 48);
		memcpy(this->title, &this->buffer[0x0134], 16);

		this->manu = this->buffer[0x014B];
		this->newManu = (this->buffer[0x0144] << 8) + this->buffer[0x0145];

		this->cgb = this->buffer[0x0143];

		this->cartType = this->buffer[0x0147];
		this->romSize = this->buffer[0x0148];
		this->ramSize = this->buffer[0x0149];

		this->lang = this->buffer[0x014A];
		this->version = this->buffer[0x014C];

		this->headerChecksum = this->buffer[0x014D];
		this->romCheckseum = (this->buffer[0x014E] << 8) + this->buffer[0x014F];

		// Compute the header checksum.
		char csum = 0;
		for (int i = 0x0134; i <= 0x014C; ++i) {
			csum = csum - this->buffer[i] - 1;
		}

		// Flag if the computed header checksum doesn't match the supplied one.
		if (csum != this->headerChecksum) {
			this->badHeaderChecksum = true;
		}

		// If this is a color gboy ROM chop the title length.
		if ((this->cgb == 0x80) || (this->cgb == 0xC0)) {
			this->title[11] = 0; this->title[12] = 0; this->title[13] = 0;
			this->title[14] = 0; this->title[15] = 0;
		}
	}

	unsigned int GetSize() const {
		return (unsigned int)this->fsize;
	}

	unsigned char* GetBuffer() const {
		return this->buffer;
	}

	std::string GetTitle() const {
		return std::string(this->title);
	}

	Byte GetType() const {
		return this->cartType;
	}



private:
	Byte* buffer;
	unsigned int fsize; // File size of the image.
	Byte ninLogo[48]; // Nin* logo
	char title[16]; // Internal title of the ROM.

	Byte manu; // Manufacturer code.
	Byte newManu; // New manufacturer code.

	Byte cgb; // Color gboy flag.

	Byte cartType; // Cartridge type.
	Byte romSize; // ROM size.
	Byte ramSize; // RAM size.

	Byte lang; //  00h Japanese, 01h Non-Japanese.
	Byte version; // Usually 0.

	Byte headerChecksum; // CHecksum of the header (verified).
	bool badHeaderChecksum; // True if the computed header checksum doesn't match the supplied one.
	Word romCheckseum; // Checksum of the whole ROM minus the checksum bytes (not verified).
};