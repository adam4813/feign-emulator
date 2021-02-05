#pragma once
#include "Binary.h"
#include "Memory.h"
#include "CPU.h"
#include <windows.h>

namespace Video {
    // Resolution is 256x256 giving us 65536 pixels.
#define RESOLUTION 102400

// Tiles are 16 bytes in size
#define TILESIZE 16

    enum VIDEO_IO_REGISTERS {
        // Drawing related registers
        LCDC = 0xFF40, // LCD Control (R/W)
        STAT = 0xFF41, // LCD Control Status (R/W)
        SCY = 0xFF42, // ScrollX (R/W)
        SCX = 0xFF43, // ScrollY (R/W)
        LY = 0xFF44, // LCD Control Y-Coordinate (R)
        LYC = 0xFF45, // LY-Compare (R/W)
        BGP = 0xFF47, // BG Palette Data (R/W)
        OBP0 = 0xFF48, // Object Palette 0 Data (R/W)
        OPB1 = 0xFF49, // Object Palette 1 Data (R/W)
        WY = 0xFF4A, // Window Y Position (R/W)
        WX = 0xFF4B, // Window X Position minus 7 (R/W)
        BGPI = 0xFF68, // Background Palette Index
        BGPD = 0xFF69, // Background Palette Data
        OBPI = 0xFF6A, // Sprite Palette Index
        OBPD = 0xFF6B, // Sprite Pallet Data
        VBK = 0xFF4F, // VRAM Bank

        // Memory related registers
        DMA = 0xFF46, // DMA Transfer and Start Address (W)
        HMDA1 = 0xFF51, // New DMA Source, High
        HMDA2 = 0xFF52, // New DMA Source, Low
        HMDA3 = 0xFF53, // New DMA Destination, High
        HMDA4 = 0xFF54, // New DMA Destination, Low
        HMDA5 = 0xFF55, // New DMA Length/Mode/Start
    };

    // Size of the various memory mappings in bytes.
#define TILEPALLET_SIZE 0x1000
#define BGMAP_SIZE 0x400
#define OAM_TABLE_SIZE 0x100

    enum VRAM {
        TILEPALLET1 = 0x8000,
        TILEPALLET2 = 0x8800,
        BGMAP1_START = 0x9800,
        BGMAP2_START = 0x9C00,
        OAM_TABLE = 0xFE00,
    };

    enum LCDCONT_BITS {
        BKGD_DISPLAY_ENABLE = BIT0, // 0
        OBJ_DISPLAY_ENABLE = BIT1, // 1
        OBJ_SIZE = BIT2, // 2
        BKGD_TILEMAP_DISPLAY_SELECT = BIT3, // 3
        BKGD_WND_TILE_DATA_SELECT = BIT4, // 4
        WND_DISPLAY_ENABLE = BIT5, // 5
        WND_TILEMAP_DISPLAY_SELECT = BIT6, // 6
        LCD_DISPLAY_ENABLE = BIT7, // 7
    };

    enum LCDSTATUS_BITS {
        MODE_FLAG_HBLANK = 0x00, // 0-1
        MODE_FLAG_VBLANK = 0x01, // 0-1
        MODE_FLAG_OAM_SEARCH = 0x02, // 0-1
        MODE_FLAG_LCD_TRANSFER = 0x03, // 0-1

        COINCIDENCE_FLAG = BIT2, // 2
        MODE0_HBLANK = BIT3, // 3
        MODE1_VBLANK = BIT4, // 4
        MODE2_OAM = BIT5, // 5
        LYC_IS_LY_COINCIDENCE = BIT6, // 6
    };

    enum OAM_ATTRIBUTE_TABLE {
        Y_POSITION = 0x000000FF, // 1.x
        X_POSITION = 0x0000FF00, // 2.x
        TILEPATTERN_NUMBER = 0x00FF0000, // 3.x
        ATTRIBUTE_FLAGS = 0xFF000000, // 4.x

        FLAG_PALETTE_NUMCGB = 0x04000000, // 4.0-2 TODO: Verify
        FLAG_TILE_VRAM_BANK = 0x08000000, // 4.3
        FLAG_PALETTE_NUM = 0x10000000, // 4.4
        FLAG_X_FLIP = 0x20000000, // 4.5
        FLAG_Y_FLIP = 0x40000000, // 4.6
        FLAG_OBJ_TO_BG = 0x80000000, // 4.7
    };

    class DMG {
    public:
        DMG(void) {
            this->screen = new char[92160];
            this->line = 0;
            this->mode = 0;
            this->modeclock = 0;
            this->scx = 0;
            this->scy = 0;
            this->lcdc = LCD_DISPLAY_ENABLE | BKGD_WND_TILE_DATA_SELECT | BKGD_DISPLAY_ENABLE;
        }

        ~DMG(void) {
            delete[] screen;
        }

        void SetRAM(Memory::MMU* r) {
            this->ram = r;

            // Store the current state in registers
            this->ram->WriteByte(LCDC, this->lcdc);
            this->ram->WriteByte(SCX, this->scx);
            this->ram->WriteByte(SCY, this->scy);
            this->ram->WriteByte(LY, this->line);
            this->ram->WriteByte(BGP, 0xFC);
            this->ram->WriteByte(OBP0, 0xFF);
            this->ram->WriteByte(OPB1, 0xFF);
            this->ram->WriteByte(WY, 0x00);
            this->ram->WriteByte(WX, 0x00);
        }

        // Set the CPU pointer.
        void SetCPU(Processor::Z80* p) {
            this->cpu = p;
        }

        Word GetTileData(int tileID, int row) {
            unsigned int offset = ((this->lcdc & BKGD_WND_TILE_DATA_SELECT) ? TILEPALLET1 : TILEPALLET2);
            // If LCDCONT.BKGD_WND_TILE_DATA_SELECT is 0 add an offset of 0x8FFF.
            Word ret = this->ram->ReadWord(offset + tileID * TILESIZE + row * 2);
            return ret;
        }

        void UpdateScreen() {
            this->scx = this->ram->ReadByte(SCX);
            this->scy = this->ram->ReadByte(SCY);
            this->pallet = this->ram->ReadByte(BGP);

            int yoffs = (this->lcdc & BKGD_TILEMAP_DISPLAY_SELECT) ? 0x1C00 : 0x1800;

            // Which line of tiles to use in the map
            yoffs += (((this->line + this->scy) & 255) >> 3) << 5;

            // Which tile to start with in the map line
            int xoffs = (this->scx >> 3);

            // Which line of pixels to use in the tiles
            int row = (this->line + this->scy) & 7;

            // Where in the tileline to start
            int column = this->scx & 7;

            int canvasoffs = this->line * 160 * 4;

            // Read tile index from the background map
            Byte tile = this->ram->ReadByte(yoffs + xoffs);

            int color[4] = { this->pallet & 0x03, this->pallet & 0x0C, this->pallet & 0x30, this->pallet & 0xC0 };

            Word tileData[8] = { GetTileData(tile, 0),
                GetTileData(tile, 1),
                GetTileData(tile, 2),
                GetTileData(tile, 3),
                GetTileData(tile, 4),
                GetTileData(tile, 5),
                GetTileData(tile, 6),
                GetTileData(tile, 7) };

            for (int i = 0; i < 160; i++) {
                // Plot the pixel to canvas
                this->screen[canvasoffs + 0] = color[0];
                this->screen[canvasoffs + 1] = color[0];
                this->screen[canvasoffs + 2] = color[0];
                this->screen[canvasoffs + 3] = color[0];
                canvasoffs += 4;

                // When this tile ends, read another
                column++;
                if (column == 8) {
                    column = 0;
                    xoffs = (xoffs + 1) & 0x1F;
                    tile = this->ram->ReadByte(yoffs + xoffs);
                    tileData[0] = GetTileData(tile, 0);
                    tileData[1] = GetTileData(tile, 1);
                    tileData[2] = GetTileData(tile, 2);
                    tileData[3] = GetTileData(tile, 3);
                    tileData[4] = GetTileData(tile, 4);
                    tileData[5] = GetTileData(tile, 5);
                    tileData[6] = GetTileData(tile, 6);
                    tileData[7] = GetTileData(tile, 7);
                }
            }
        }

        void Step() {
            this->lcdc = this->ram->ReadByte(LCDC);
            static int lastT = 0;
            this->modeclock += this->cpu->GetTotalT() - lastT;
            lastT = this->cpu->GetTotalT();

            switch (this->mode) {
            case MODE_FLAG_HBLANK:
            if (this->modeclock >= 204) {
                this->line++;

                if (this->line == 143) {
                    Byte interrupts = this->ram->ReadByte(0xFFFF);
                    if (interrupts & Processor::INTERRUPTS::VBLANK) {
                        Byte interruptsFlag = this->ram->ReadByte(0xFF0F);
                        this->ram->WriteByte(0xFF0F, interruptsFlag |= Processor::INTERRUPTS::VBLANK);
                    }
                    this->mode = MODE_FLAG_VBLANK;
                    this->ram->WriteByte(STAT, 0xFF & (MODE_FLAG_HBLANK | MODE1_VBLANK));
                }
                else {
                    this->mode = MODE_FLAG_OAM_SEARCH;
                    this->ram->WriteByte(STAT, 0xFF & (MODE_FLAG_OAM_SEARCH | MODE2_OAM));
                }
                this->ram->WriteByte(LY, this->line);
                this->modeclock -= 204;
            }
            break;
            case MODE_FLAG_VBLANK:
            if (this->modeclock >= 456) {
                this->line++;

                if (this->line > 153) {
                    // Restart scanning modes
                    this->mode = MODE_FLAG_OAM_SEARCH;
                    this->ram->WriteByte(STAT, 0xFF & (MODE_FLAG_OAM_SEARCH | MODE2_OAM));
                    this->line = 0;
                }
                this->ram->WriteByte(LY, this->line);
                this->modeclock -= 456;
            }
            break;
            case MODE_FLAG_OAM_SEARCH:
            if (this->modeclock >= 80) {
                this->mode = MODE_FLAG_LCD_TRANSFER;
                this->ram->WriteByte(STAT, 0xFF & MODE_FLAG_LCD_TRANSFER);
                this->modeclock -= 80;
            }
            break;
            case MODE_FLAG_LCD_TRANSFER:
            if (this->modeclock >= 172) {
                this->mode = MODE_FLAG_HBLANK;
                this->ram->WriteByte(STAT, 0xFF & (MODE_FLAG_HBLANK | MODE0_HBLANK));
                this->modeclock -= 172;

                UpdateScreen();
            }
            break;
            default:
            break;
            }
            this->ram->WriteByte(STAT, this->lcdc);
        }


    private:
        char* screen;

        char tiles[512][8][8];

        Memory::MMU* ram;
        Processor::Z80* cpu;

        Byte mode;
        int modeclock;
        Byte line;
        Byte lcdc;
        Byte scy;
        Byte scx;
        Byte pallet;
    };
}
