#include "../include/CPU.h"

#include <iostream>
#include <windows.h>
#include <map>

#include <chrono>
#include <thread>

using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::chrono::high_resolution_clock;
using std::chrono::duration;

high_resolution_clock::time_point start;
high_resolution_clock::time_point end;
std::map<Byte, long long> timings;
std::map<Byte, int> calls;

// Reference for comments above each function http://imrannazar.com/content/files/jsgb.z80.js

namespace Processor {
    Z80::Z80() {
        this->PC = 0; this->SP.word = 0xFFFE;
        this->HL.word = 0x014D; this->BC.last = 0x13;
        this->DE.last = 0xD8; this->AF.word = 0x01B0;

        this->M = 0; this->T = 0;
        this->total_M = 0; this->total_T = 0;

        this->stop = false;
        this->halt = false;
        this->IME = true;

        this->numInstructions = 0;
    }

    Z80::~Z80() {

    }

    void Z80::SetMMU(Memory::MMU* r) {
        this->ram = r;

        this->ram->WriteByte(0xFFFF, 0);
        this->ram->WriteByte(0xFF0F, 0);
    }

    void Z80::SetPC(Word address) {
        this->PC = address;
    }

    Word Z80::GetPC() {
        return this->PC;
    }

    int Z80::GetT() {
        return this->T;
    }

    int Z80::GetTotalT() {
        return this->total_T;
    }

    void Z80::DoInterrupts() {
        Byte interrupts = this->ram->ReadByte(0xFFFF);
        Byte interruptsFlag = this->ram->ReadByte(0xFF0F);
        //std::cout << this->IME << " " << interrupts << " " << interruptsFlag << std::endl;

        if (this->IME) { // && interrupts && interruptsFlag) {
            if (interrupts == 0) {
                return;
            }
            unsigned char fire = interruptsFlag & interrupts; // mask enabled interrupt(s)
            this->IME = false;

            this->halt = false;

            if (fire & VBLANK) {
                interruptsFlag &= ~VBLANK;
                // draw screen
                RST_N(40);
            }
            else if (fire & LCDC_STATUS) {
                interruptsFlag &= ~LCDC_STATUS;

                RST_N(48);
            }
            else if (fire & TIMER_OVERFLOW) {
                interruptsFlag &= ~TIMER_OVERFLOW;

                RST_N(50);
            }
            else if (fire & SERIAL_LINK) {
                interruptsFlag &= ~SERIAL_LINK;

                RST_N(58);
            }
            else if (fire & JOYPAD_LINK) {
                interruptsFlag &= ~JOYPAD_LINK;

                RST_N(60);
            }
            else {
                this->IME = true;
            }
        }

        this->ram->WriteByte(0xFFFF, interrupts);
        this->ram->WriteByte(0xFF0F, interruptsFlag);

        this->T += 12;
        this->total_T += this->T;
    }

    bool Z80::DoNextOp() {
        start = high_resolution_clock::now();

        Byte op = this->ram->ReadByte(this->PC);

        ++this->numInstructions;
        ++this->PC;

        switch (op) {
            // 00-0F
        case 0x00: NOP(); break;
        case 0x01: LD_RR_NN(&this->BC); break;
        case 0x02: LD_BCm_A(); break;
        case 0x03: INC_RR(&this->BC); break;
        case 0x04: INC_R(&this->BC.first); break;
        case 0x05: DEC_R(&this->BC.first); break;
        case 0x06: LD_R_N(&this->BC.first); break;
        case 0x07: RLCA(); break;
        case 0x08: LD_NNm_SP(); break;
        case 0x09: ADD_HL_RR(&this->BC); break;
        case 0x0A: LD_A_BCm(); break;
        case 0x0B: DEC_RR(&this->BC); break;
        case 0x0C: INC_R(&this->BC.last); break;
        case 0x0D: DEC_R(&this->BC.last); break;
        case 0x0E: LD_R_N(&this->BC.last); break;
        case 0x0F: RRCA(); break;

            // 10-1F
        case 0x10: STOP(); break;
        case 0x11: LD_RR_NN(&this->DE); break;
        case 0x12: LD_DEm_A(); break;
        case 0x13: INC_RR(&this->DE); break;
        case 0x14: INC_R(&this->DE.first); break;
        case 0x15: DEC_R(&this->DE.first); break;
        case 0x16: LD_R_N(&this->DE.first); break;
        case 0x17: RLA(); break;
        case 0x18: JR_PCdd(); break;
        case 0x19: ADD_HL_RR(&this->DE); break;
        case 0x1A: LD_A_DEm(); break;
        case 0x1B: DEC_RR(&this->DE); break;
        case 0x1C: INC_R(&this->DE.last); break;
        case 0x1D: DEC_R(&this->DE.last); break;
        case 0x1E: LD_R_N(&this->DE.last); break;
        case 0x1F: RRA(); break;

            // 20-2F
        case 0x20: JR_NOTF_PCdd(zf); break;
        case 0x21: LD_RR_NN(&this->HL); break;
        case 0x22: LDInc_HLm_A(); break;
        case 0x23: INC_RR(&this->HL); break;
        case 0x24: INC_R(&this->HL.first); break;
        case 0x25: DEC_R(&this->HL.first); break;
        case 0x26: LD_R_N(&this->HL.first); break;
        case 0x27: DAA(); break;
        case 0x28: JR_F_PCdd(zf); break;
        case 0x29: ADD_HL_RR(&this->HL); break;
        case 0x2A: LDInc_A_HLm(); break;
        case 0x2B: DEC_RR(&this->HL); break;
        case 0x2C: INC_R(&this->HL.last); break;
        case 0x2D: DEC_R(&this->HL.last); break;
        case 0x2E: LD_R_N(&this->HL.last); break;
        case 0x2F: CPL(); break;

            // 30-3F
        case 0x30: JR_NOTF_PCdd(cy); break;
        case 0x31: LD_RR_NN(&this->SP); break;
        case 0x32: LDDec_HLm_A(); break;
        case 0x33: INC_RR(&this->SP); break;
        case 0x34: INC_HLm(); break;
        case 0x35: DEC_HLm(); break;
        case 0x36: LD_HLm_N(); break;
        case 0x37: SCF(); break;
        case 0x38: JR_F_PCdd(cy); break;
        case 0x39: ADD_HL_RR(&this->SP); break;
        case 0x3A: LDDec_A_HLm(); break;
        case 0x3B: DEC_RR(&this->SP); break;
        case 0x3C: INC_R(&this->AF.first); break;
        case 0x3D: DEC_R(&this->AF.first); break;
        case 0x3E: LD_R_N(&this->AF.first); break;
        case 0x3F: CCF(); break;

            // 40-4F
        case 0x40: LD_R_R(&this->BC.first, &this->BC.first); break;
        case 0x41: LD_R_R(&this->BC.first, &this->BC.last); break;
        case 0x42: LD_R_R(&this->BC.first, &this->DE.first); break;
        case 0x43: LD_R_R(&this->BC.first, &this->DE.last); break;
        case 0x44: LD_R_R(&this->BC.first, &this->HL.first); break;
        case 0x45: LD_R_R(&this->BC.first, &this->HL.last); break;
        case 0x46: LD_R_HLm(&this->BC.first); break;
        case 0x47: LD_R_R(&this->BC.first, &this->AF.first); break;
        case 0x48: LD_R_R(&this->BC.last, &this->BC.first); break;
        case 0x49: LD_R_R(&this->BC.last, &this->BC.last); break;
        case 0x4A: LD_R_R(&this->BC.last, &this->DE.first); break;
        case 0x4B: LD_R_R(&this->BC.last, &this->DE.last); break;
        case 0x4C: LD_R_R(&this->BC.last, &this->HL.first); break;
        case 0x4D: LD_R_R(&this->BC.last, &this->HL.last); break;
        case 0x4E: LD_R_HLm(&this->BC.last); break;
        case 0x4F: LD_R_R(&this->BC.last, &this->AF.first); break;

            // 50-5F
        case 0x50: LD_R_R(&this->DE.first, &this->BC.first); break;
        case 0x51: LD_R_R(&this->DE.first, &this->BC.last); break;
        case 0x52: LD_R_R(&this->DE.first, &this->DE.first); break;
        case 0x53: LD_R_R(&this->DE.first, &this->DE.last); break;
        case 0x54: LD_R_R(&this->DE.first, &this->HL.first); break;
        case 0x55: LD_R_R(&this->DE.first, &this->HL.last); break;
        case 0x56: LD_R_HLm(&this->DE.first); break;
        case 0x57: LD_R_R(&this->DE.first, &this->AF.first); break;
        case 0x58: LD_R_R(&this->DE.last, &this->BC.first); break;
        case 0x59: LD_R_R(&this->DE.last, &this->BC.last); break;
        case 0x5A: LD_R_R(&this->DE.last, &this->DE.first); break;
        case 0x5B: LD_R_R(&this->DE.last, &this->DE.last); break;
        case 0x5C: LD_R_R(&this->DE.last, &this->HL.first); break;
        case 0x5D: LD_R_R(&this->DE.last, &this->HL.last); break;
        case 0x5E: LD_R_HLm(&this->DE.last); break;
        case 0x5F: LD_R_R(&this->DE.last, &this->AF.first); break;

            // 60-6F
        case 0x60: LD_R_R(&this->HL.first, &this->BC.first); break;
        case 0x61: LD_R_R(&this->HL.first, &this->BC.last); break;
        case 0x62: LD_R_R(&this->HL.first, &this->DE.first); break;
        case 0x63: LD_R_R(&this->HL.first, &this->DE.last); break;
        case 0x64: LD_R_R(&this->HL.first, &this->HL.first); break;
        case 0x65: LD_R_R(&this->HL.first, &this->HL.last); break;
        case 0x66: LD_R_HLm(&this->HL.first); break;
        case 0x67: LD_R_R(&this->HL.first, &this->AF.first); break;
        case 0x68: LD_R_R(&this->HL.last, &this->BC.first); break;
        case 0x69: LD_R_R(&this->HL.last, &this->BC.last); break;
        case 0x6A: LD_R_R(&this->HL.last, &this->DE.first); break;
        case 0x6B: LD_R_R(&this->HL.last, &this->DE.last); break;
        case 0x6C: LD_R_R(&this->HL.last, &this->HL.first); break;
        case 0x6D: LD_R_R(&this->HL.last, &this->HL.last); break;
        case 0x6E: LD_R_HLm(&this->HL.last); break;
        case 0x6F: LD_R_R(&this->HL.last, &this->AF.first); break;

            // 70-7F
        case 0x70: LD_HLm_R(&this->BC.first); break;
        case 0x71: LD_HLm_R(&this->BC.last); break;
        case 0x72: LD_HLm_R(&this->DE.first); break;
        case 0x73: LD_HLm_R(&this->DE.last); break;
        case 0x74: LD_HLm_R(&this->HL.first); break;
        case 0x75: LD_HLm_R(&this->HL.last); break;
        case 0x76: HALT(); break;
        case 0x77: LD_HLm_R(&this->AF.first); break;
        case 0x78: LD_R_R(&this->AF.first, &this->BC.first); break;
        case 0x79: LD_R_R(&this->AF.first, &this->BC.last); break;
        case 0x7A: LD_R_R(&this->AF.first, &this->DE.first); break;
        case 0x7B: LD_R_R(&this->AF.first, &this->DE.last); break;
        case 0x7C: LD_R_R(&this->AF.first, &this->HL.first); break;
        case 0x7D: LD_R_R(&this->AF.first, &this->HL.last); break;
        case 0x7E: LD_R_HLm(&this->AF.first); break;
        case 0x7F: LD_R_R(&this->AF.first, &this->HL.first); break;


            // 80-8F
        case 0x80: ADD_A_R(&this->BC.first); break;
        case 0x81: ADD_A_R(&this->BC.last); break;
        case 0x82: ADD_A_R(&this->DE.first); break;
        case 0x83: ADD_A_R(&this->DE.last); break;
        case 0x84: ADD_A_R(&this->HL.first); break;
        case 0x85: ADD_A_R(&this->HL.last); break;
        case 0x86: ADD_A_HLm(); break;
        case 0x87: ADD_A_R(&this->HL.first); break;
        case 0x88: ADC_A_R(&this->BC.first); break;
        case 0x89: ADC_A_R(&this->BC.last); break;
        case 0x8A: ADC_A_R(&this->DE.first); break;
        case 0x8B: ADC_A_R(&this->DE.last); break;
        case 0x8C: ADC_A_R(&this->HL.first); break;
        case 0x8D: ADC_A_R(&this->HL.last); break;
        case 0x8E: ADC_A_HLm(); break;
        case 0x8F: ADC_A_R(&this->AF.first); break;

            // 90-9F
        case 0x90: SUB_A_R(&this->BC.first); break;
        case 0x91: SUB_A_R(&this->BC.last); break;
        case 0x92: SUB_A_R(&this->DE.first); break;
        case 0x93: SUB_A_R(&this->DE.last); break;
        case 0x94: SUB_A_R(&this->HL.first); break;
        case 0x95: SUB_A_R(&this->HL.last); break;
        case 0x96: SUB_A_HLm(); break;
        case 0x97: SUB_A_R(&this->HL.first); break;
        case 0x98: SBC_A_R(&this->BC.first); break;
        case 0x99: SBC_A_R(&this->BC.last); break;
        case 0x9A: SBC_A_R(&this->DE.first); break;
        case 0x9B: SBC_A_R(&this->DE.last); break;
        case 0x9C: SBC_A_R(&this->HL.first); break;
        case 0x9D: SBC_A_R(&this->HL.last); break;
        case 0x9E: SBC_A_HLm(); break;
        case 0x9F: SBC_A_R(&this->AF.first); break;

            // A0-AF
        case 0xA0: AND_A_R(&this->BC.first); break;
        case 0xA1: AND_A_R(&this->BC.last); break;
        case 0xA2: AND_A_R(&this->DE.first); break;
        case 0xA3: AND_A_R(&this->DE.last); break;
        case 0xA4: AND_A_R(&this->HL.first); break;
        case 0xA5: AND_A_R(&this->HL.last); break;
        case 0xA6: AND_A_HLm(); break;
        case 0xA7: AND_A_R(&this->HL.first); break;
        case 0xA8: XOR_A_R(&this->BC.first); break;
        case 0xA9: XOR_A_R(&this->BC.last); break;
        case 0xAA: XOR_A_R(&this->DE.first); break;
        case 0xAB: XOR_A_R(&this->DE.last); break;
        case 0xAC: XOR_A_R(&this->HL.first); break;
        case 0xAD: XOR_A_R(&this->HL.last); break;
        case 0xAE: XOR_A_HLm(); break;
        case 0xAF: XOR_A_R(&this->AF.first); break;

            // B0-BF
        case 0xB0: OR_A_R(&this->BC.first); break;
        case 0xB1: OR_A_R(&this->BC.last); break;
        case 0xB2: OR_A_R(&this->DE.first); break;
        case 0xB3: OR_A_R(&this->DE.last); break;
        case 0xB4: OR_A_R(&this->HL.first); break;
        case 0xB5: OR_A_R(&this->HL.last); break;
        case 0xB6: OR_A_HLm(); break;
        case 0xB7: OR_A_R(&this->HL.first); break;
        case 0xB8: CP_A_R(&this->BC.first); break;
        case 0xB9: CP_A_R(&this->BC.last); break;
        case 0xBA: CP_A_R(&this->DE.first); break;
        case 0xBB: CP_A_R(&this->DE.last); break;
        case 0xBC: CP_A_R(&this->HL.first); break;
        case 0xBD: CP_A_R(&this->HL.last); break;
        case 0xBE: CP_A_HLm(); break;
        case 0xBF: CP_A_R(&this->AF.first); break;

            // C0-CF
        case 0xC0: RET_NOTF(zf); break;
        case 0xC1: POP_RR(&this->BC); break;
        case 0xC2: JP_NOTF_NN(zf); break;
        case 0xC3: JP_NN(); break;
        case 0xC4: CALL_NOTF_NN(zf); break;
        case 0xC5: PUSH_RR(&this->BC); break;
        case 0xC6: ADD_A_N(); break;
        case 0xC7: RST_N(0x00); break;
        case 0xC8: RET_F(zf); break;
        case 0xC9: RET(); break;
        case 0xCA: JP_F_NN(zf); break;
        case 0xCB: DoCBOp(); break;
        case 0xCC: CALL_F_NN(zf); break;
        case 0xCD: CALL_NN(); break;
        case 0xCE: ADC_A_N(); break;
        case 0xCF: RST_N(0x08); break;

            // D0-DF
        case 0xD0: RET_NOTF(cy); break;
        case 0xD1: POP_RR(&this->DE); break;
        case 0xD2: JP_NOTF_NN(cy); break;
        case 0xD3: NOP(); break;
        case 0xD4: CALL_NOTF_NN(cy); break;
        case 0xD5: PUSH_RR(&this->DE); break;
        case 0xD6: SUB_A_N(); break;
        case 0xD7: RST_N(0x10); break;
        case 0xD8: RET_F(cy); break;
        case 0xD9: RETI(); break;
        case 0xDA: JP_F_NN(cy); break;
        case 0xDB: NOP(); break;
        case 0xDC: CALL_F_NN(cy); break;
        case 0xDD: NOP(); break;
        case 0xDE: SBC_A_N(); break;
        case 0xDF: RST_N(0x18); break;

            // E0-EF
        case 0xE0: LD_IONm_A(); break;
        case 0xE1: POP_RR(&this->HL); break;
        case 0xE2: LD_IOCm_A(); break;
        case 0xE3: NOP(); break;
        case 0xE4: NOP(); break;
        case 0xE5: PUSH_RR(&this->HL); break;
        case 0xE6: AND_A_N(); break;
        case 0xE7: RST_N(0x20); break;
        case 0xE8: ADD_SP_dd(); break;
        case 0xE9: JP_HL(); break;
        case 0xEA: LD_NNm_A(); break;
        case 0xEB: NOP(); break;
        case 0xEC: NOP(); break;
        case 0xED: NOP(); break;
        case 0xEE: XOR_A_N(); break;
        case 0xEF: RST_N(0x28); break;

            // F0-FF
        case 0xF0: LD_A_IONm(); break;
        case 0xF1: POP_RR(&this->AF); break;
        case 0xF2: LD_A_IOCm(); break;
        case 0xF3: DI(); break;
        case 0xF4: NOP(); break;
        case 0xF5: PUSH_RR(&this->AF); break;
        case 0xF6: OR_A_N(); break;
        case 0xF7: RST_N(0x30); break;
        case 0xF8: LD_HL_SPdd(); break;
        case 0xF9: LD_SP_HL(); break;
        case 0xFA: LD_A_NNm(); break;
        case 0xFB: EI(); break;
        case 0xFC: NOP(); break;
        case 0xFD: NOP(); break;
        case 0xFE: CP_A_N(); break;
        case 0xFF: RST_N(0x38); break;

        default:
        break;
        }

        end = high_resolution_clock::now();

        if (this->halt) {
            return false;
        }

        if (this->stop) {
            return false;
        }

        this->total_M += this->M;
        this->total_T += this->T;

        return true;
    }

    void Z80::DoCBOp() {
        Byte op = this->ram->ReadByte(this->PC);

        ++this->PC;

        switch (op) {
            // 00-0F
        case 0x00: RLC(&this->BC.first); break;
        case 0x01: RLC(&this->BC.last); break;
        case 0x02: RLC(&this->DE.first); break;
        case 0x03: RLC(&this->DE.last); break;
        case 0x04: RLC(&this->HL.first); break;
        case 0x05: RLC(&this->HL.last); break;
        case 0x06: RLC_HL(); break;
        case 0x07: RLC(&this->AF.first); break;
        case 0x08: RRC(&this->BC.first); break;
        case 0x09: RRC(&this->BC.last); break;
        case 0x0A: RRC(&this->DE.first); break;
        case 0x0B: RRC(&this->DE.last); break;
        case 0x0C: RRC(&this->HL.first); break;
        case 0x0D: RRC(&this->HL.last); break;
        case 0x0E: RRC_HL(); break;
        case 0x0F: RRC(&this->AF.first); break;

            // 10-1F
        case 0x10: RL(&this->BC.first); break;
        case 0x11: RL(&this->BC.last); break;
        case 0x12: RL(&this->DE.first); break;
        case 0x13: RL(&this->DE.last); break;
        case 0x14: RL(&this->HL.first); break;
        case 0x15: RL(&this->HL.last); break;
        case 0x16: RL_HL(); break;
        case 0x17: RL(&this->AF.first); break;
        case 0x18: RR(&this->BC.first); break;
        case 0x19: RR(&this->BC.last); break;
        case 0x1A: RR(&this->DE.first); break;
        case 0x1B: RR(&this->DE.last); break;
        case 0x1C: RR(&this->HL.first); break;
        case 0x1D: RR(&this->HL.last); break;
        case 0x1E: RR_HL(); break;
        case 0x1F: RR(&this->AF.first); break;

            // 20-2F
        case 0x20: SLA(&this->BC.first); break;
        case 0x21: SLA(&this->BC.last); break;
        case 0x22: SLA(&this->DE.first); break;
        case 0x23: SLA(&this->DE.last); break;
        case 0x24: SLA(&this->HL.first); break;
        case 0x25: SLA(&this->HL.last); break;
        case 0x26: SLA_HL(); break;
        case 0x27: SRA(&this->AF.first); break;
        case 0x28: SRA(&this->BC.first); break;
        case 0x29: SRA(&this->BC.last); break;
        case 0x2A: SRA(&this->DE.first); break;
        case 0x2B: SRA(&this->DE.last); break;
        case 0x2C: SRA(&this->HL.first); break;
        case 0x2D: SRA(&this->HL.last); break;
        case 0x2E: SRA_HL(); break;
        case 0x2F: SRA(&this->AF.first); break;

            // 30-3F
        case 0x30: SWAP(&this->BC.first); break;
        case 0x31: SWAP(&this->BC.last); break;
        case 0x32: SWAP(&this->DE.first); break;
        case 0x33: SWAP(&this->DE.last); break;
        case 0x34: SWAP(&this->HL.first); break;
        case 0x35: SWAP(&this->HL.last); break;
        case 0x36: SWAP_HL(); break;
        case 0x37: SRL(&this->AF.first); break;
        case 0x38: SRL(&this->BC.first); break;
        case 0x39: SRL(&this->BC.last); break;
        case 0x3A: SRL(&this->DE.first); break;
        case 0x3B: SRL(&this->DE.last); break;
        case 0x3C: SRL(&this->HL.first); break;
        case 0x3D: SRL(&this->HL.last); break;
        case 0x3E: SRL_HL(); break;
        case 0x3F: SRL(&this->AF.first); break;

            // 40-4F
        case 0x40: BITTEST(&this->BC.first, 0); break;
        case 0x41: BITTEST(&this->BC.last, 0); break;
        case 0x42: BITTEST(&this->DE.first, 0); break;
        case 0x43: BITTEST(&this->DE.last, 0); break;
        case 0x44: BITTEST(&this->HL.first, 0); break;
        case 0x45: BITTEST(&this->HL.last, 0); break;
        case 0x46: BITTEST_HL(0); break;
        case 0x47: BITTEST(&this->AF.first, 0); break;
        case 0x48: BITTEST(&this->BC.first, 1); break;
        case 0x49: BITTEST(&this->BC.last, 1); break;
        case 0x4A: BITTEST(&this->DE.first, 1); break;
        case 0x4B: BITTEST(&this->DE.last, 1); break;
        case 0x4C: BITTEST(&this->HL.first, 1); break;
        case 0x4D: BITTEST(&this->HL.last, 1); break;
        case 0x4E: BITTEST_HL(1); break;
        case 0x4F: BITTEST(&this->AF.first, 1); break;

            // 50-5F
        case 0x50: BITTEST(&this->BC.first, 2); break;
        case 0x51: BITTEST(&this->BC.last, 2); break;
        case 0x52: BITTEST(&this->DE.first, 2); break;
        case 0x53: BITTEST(&this->DE.last, 2); break;
        case 0x54: BITTEST(&this->HL.first, 2); break;
        case 0x55: BITTEST(&this->HL.last, 2); break;
        case 0x56: BITTEST_HL(2); break;
        case 0x57: BITTEST(&this->AF.first, 2); break;
        case 0x58: BITTEST(&this->BC.first, 3); break;
        case 0x59: BITTEST(&this->BC.last, 3); break;
        case 0x5A: BITTEST(&this->DE.first, 3); break;
        case 0x5B: BITTEST(&this->DE.last, 3); break;
        case 0x5C: BITTEST(&this->HL.first, 3); break;
        case 0x5D: BITTEST(&this->HL.last, 3); break;
        case 0x5E: BITTEST_HL(3); break;
        case 0x5F: BITTEST(&this->AF.first, 3); break;

            // 60-6F
        case 0x60: BITTEST(&this->BC.first, 4); break;
        case 0x61: BITTEST(&this->BC.last, 4); break;
        case 0x62: BITTEST(&this->DE.first, 4); break;
        case 0x63: BITTEST(&this->DE.last, 4); break;
        case 0x64: BITTEST(&this->HL.first, 4); break;
        case 0x65: BITTEST(&this->HL.last, 4); break;
        case 0x66: BITTEST_HL(4); break;
        case 0x67: BITTEST(&this->AF.first, 4); break;
        case 0x68: BITTEST(&this->BC.first, 5); break;
        case 0x69: BITTEST(&this->BC.last, 5); break;
        case 0x6A: BITTEST(&this->DE.first, 5); break;
        case 0x6B: BITTEST(&this->DE.last, 5); break;
        case 0x6C: BITTEST(&this->HL.first, 5); break;
        case 0x6D: BITTEST(&this->HL.last, 5); break;
        case 0x6E: BITTEST_HL(5); break;
        case 0x6F: BITTEST(&this->AF.first, 5); break;

            // 70-7F
        case 0x70: BITTEST(&this->BC.first, 6); break;
        case 0x71: BITTEST(&this->BC.last, 6); break;
        case 0x72: BITTEST(&this->DE.first, 6); break;
        case 0x73: BITTEST(&this->DE.last, 6); break;
        case 0x74: BITTEST(&this->HL.first, 6); break;
        case 0x75: BITTEST(&this->HL.last, 6); break;
        case 0x76: BITTEST_HL(6); break;
        case 0x77: BITTEST(&this->AF.first, 6); break;
        case 0x78: BITTEST(&this->BC.first, 7); break;
        case 0x79: BITTEST(&this->BC.last, 7); break;
        case 0x7A: BITTEST(&this->DE.first, 7); break;
        case 0x7B: BITTEST(&this->DE.last, 7); break;
        case 0x7C: BITTEST(&this->HL.first, 7); break;
        case 0x7D: BITTEST(&this->HL.last, 7); break;
        case 0x7E: BITTEST_HL(7); break;
        case 0x7F: BITTEST(&this->AF.first, 7); break;


            // 80-8F
        case 0x80: CLEARBIT(&this->BC.first, 0); break;
        case 0x81: CLEARBIT(&this->BC.last, 0); break;
        case 0x82: CLEARBIT(&this->DE.first, 0); break;
        case 0x83: CLEARBIT(&this->DE.last, 0); break;
        case 0x84: CLEARBIT(&this->HL.first, 0); break;
        case 0x85: CLEARBIT(&this->HL.last, 0); break;
        case 0x86: CLEARBIT_HL(0); break;
        case 0x87: CLEARBIT(&this->AF.first, 0); break;
        case 0x88: CLEARBIT(&this->BC.first, 1); break;
        case 0x89: CLEARBIT(&this->BC.last, 1); break;
        case 0x8A: CLEARBIT(&this->DE.first, 1); break;
        case 0x8B: CLEARBIT(&this->DE.last, 1); break;
        case 0x8C: CLEARBIT(&this->HL.first, 1); break;
        case 0x8D: CLEARBIT(&this->HL.last, 1); break;
        case 0x8E: CLEARBIT_HL(1); break;
        case 0x8F: CLEARBIT(&this->AF.first, 1); break;

            // 90-9F
        case 0x90: CLEARBIT(&this->BC.first, 2); break;
        case 0x91: CLEARBIT(&this->BC.last, 2); break;
        case 0x92: CLEARBIT(&this->DE.first, 2); break;
        case 0x93: CLEARBIT(&this->DE.last, 2); break;
        case 0x94: CLEARBIT(&this->HL.first, 2); break;
        case 0x95: CLEARBIT(&this->HL.last, 2); break;
        case 0x96: CLEARBIT_HL(2); break;
        case 0x97: CLEARBIT(&this->AF.first, 2); break;
        case 0x98: CLEARBIT(&this->BC.first, 3); break;
        case 0x99: CLEARBIT(&this->BC.last, 3); break;
        case 0x9A: CLEARBIT(&this->DE.first, 3); break;
        case 0x9B: CLEARBIT(&this->DE.last, 3); break;
        case 0x9C: CLEARBIT(&this->HL.first, 3); break;
        case 0x9D: CLEARBIT(&this->HL.last, 3); break;
        case 0x9E: CLEARBIT_HL(3); break;
        case 0x9F: CLEARBIT(&this->AF.first, 3); break;

            // A0-AF
        case 0xA0: CLEARBIT(&this->BC.first, 4); break;
        case 0xA1: CLEARBIT(&this->BC.last, 4); break;
        case 0xA2: CLEARBIT(&this->DE.first, 4); break;
        case 0xA3: CLEARBIT(&this->DE.last, 4); break;
        case 0xA4: CLEARBIT(&this->HL.first, 4); break;
        case 0xA5: CLEARBIT(&this->HL.last, 4); break;
        case 0xA6: CLEARBIT_HL(4); break;
        case 0xA7: CLEARBIT(&this->AF.first, 4); break;
        case 0xA8: CLEARBIT(&this->BC.first, 5); break;
        case 0xA9: CLEARBIT(&this->BC.last, 5); break;
        case 0xAA: CLEARBIT(&this->DE.first, 5); break;
        case 0xAB: CLEARBIT(&this->DE.last, 5); break;
        case 0xAC: CLEARBIT(&this->HL.first, 5); break;
        case 0xAD: CLEARBIT(&this->HL.last, 5); break;
        case 0xAE: CLEARBIT_HL(5); break;
        case 0xAF: CLEARBIT(&this->AF.first, 5); break;

            // B0-BF
        case 0xB0: CLEARBIT(&this->BC.first, 6); break;
        case 0xB1: CLEARBIT(&this->BC.last, 6); break;
        case 0xB2: CLEARBIT(&this->DE.first, 6); break;
        case 0xB3: CLEARBIT(&this->DE.last, 6); break;
        case 0xB4: CLEARBIT(&this->HL.first, 6); break;
        case 0xB5: CLEARBIT(&this->HL.last, 6); break;
        case 0xB6: CLEARBIT_HL(6); break;
        case 0xB7: CLEARBIT(&this->AF.first, 6); break;
        case 0xB8: CLEARBIT(&this->BC.first, 7); break;
        case 0xB9: CLEARBIT(&this->BC.last, 7); break;
        case 0xBA: CLEARBIT(&this->DE.first, 7); break;
        case 0xBB: CLEARBIT(&this->DE.last, 7); break;
        case 0xBC: CLEARBIT(&this->HL.first, 7); break;
        case 0xBD: CLEARBIT(&this->HL.last, 7); break;
        case 0xBE: CLEARBIT_HL(7); break;
        case 0xBF: CLEARBIT(&this->AF.first, 7); break;

            // C0-CF
        case 0xC0: SETBIT(&this->BC.first, 0); break;
        case 0xC1: SETBIT(&this->BC.last, 0); break;
        case 0xC2: SETBIT(&this->DE.first, 0); break;
        case 0xC3: SETBIT(&this->DE.last, 0); break;
        case 0xC4: SETBIT(&this->HL.first, 0); break;
        case 0xC5: SETBIT(&this->HL.last, 0); break;
        case 0xC6: SETBIT_HL(0); break;
        case 0xC7: SETBIT(&this->AF.first, 0); break;
        case 0xC8: SETBIT(&this->BC.first, 1); break;
        case 0xC9: SETBIT(&this->BC.last, 1); break;
        case 0xCA: SETBIT(&this->DE.first, 1); break;
        case 0xCB: SETBIT(&this->DE.last, 1); break;
        case 0xCC: SETBIT(&this->HL.first, 1); break;
        case 0xCD: SETBIT(&this->HL.last, 1); break;
        case 0xCE: SETBIT_HL(1); break;
        case 0xCF: SETBIT(&this->AF.first, 1); break;

            // D0-DF
        case 0xD0: SETBIT(&this->BC.first, 2); break;
        case 0xD1: SETBIT(&this->BC.last, 2); break;
        case 0xD2: SETBIT(&this->DE.first, 2); break;
        case 0xD3: SETBIT(&this->DE.last, 2); break;
        case 0xD4: SETBIT(&this->HL.first, 2); break;
        case 0xD5: SETBIT(&this->HL.last, 2); break;
        case 0xD6: SETBIT_HL(2); break;
        case 0xD7: SETBIT(&this->AF.first, 2); break;
        case 0xD8: SETBIT(&this->BC.first, 3); break;
        case 0xD9: SETBIT(&this->BC.last, 3); break;
        case 0xDA: SETBIT(&this->DE.first, 3); break;
        case 0xDB: SETBIT(&this->DE.last, 3); break;
        case 0xDC: SETBIT(&this->HL.first, 3); break;
        case 0xDD: SETBIT(&this->HL.last, 3); break;
        case 0xDE: SETBIT_HL(3); break;
        case 0xDF: SETBIT(&this->AF.first, 3); break;

            // E0-EF
        case 0xE0: SETBIT(&this->BC.first, 4); break;
        case 0xE1: SETBIT(&this->BC.last, 4); break;
        case 0xE2: SETBIT(&this->DE.first, 4); break;
        case 0xE3: SETBIT(&this->DE.last, 4); break;
        case 0xE4: SETBIT(&this->HL.first, 4); break;
        case 0xE5: SETBIT(&this->HL.last, 4); break;
        case 0xE6: SETBIT_HL(4); break;
        case 0xE7: SETBIT(&this->AF.first, 4); break;
        case 0xE8: SETBIT(&this->BC.first, 5); break;
        case 0xE9: SETBIT(&this->BC.last, 5); break;
        case 0xEA: SETBIT(&this->DE.first, 5); break;
        case 0xEB: SETBIT(&this->DE.last, 5); break;
        case 0xEC: SETBIT(&this->HL.first, 5); break;
        case 0xED: SETBIT(&this->HL.last, 5); break;
        case 0xEE: SETBIT_HL(5); break;
        case 0xEF: SETBIT(&this->AF.first, 5); break;

            // F0-FF
        case 0xF0: SETBIT(&this->BC.first, 6); break;
        case 0xF1: SETBIT(&this->BC.last, 6); break;
        case 0xF2: SETBIT(&this->DE.first, 6); break;
        case 0xF3: SETBIT(&this->DE.last, 6); break;
        case 0xF4: SETBIT(&this->HL.first, 6); break;
        case 0xF5: SETBIT(&this->HL.last, 6); break;
        case 0xF6: SETBIT_HL(6); break;
        case 0xF7: SETBIT(&this->AF.first, 6); break;
        case 0xF8: SETBIT(&this->BC.first, 7); break;
        case 0xF9: SETBIT(&this->BC.last, 7); break;
        case 0xFA: SETBIT(&this->DE.first, 7); break;
        case 0xFB: SETBIT(&this->DE.last, 7); break;
        case 0xFC: SETBIT(&this->HL.first, 7); break;
        case 0xFD: SETBIT(&this->HL.last, 7); break;
        case 0xFE: SETBIT_HL(7); break;
        case 0xFF: SETBIT(&this->AF.first, 7); break;

        default:
        break;
        }
    }

    void Z80::RLC(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RLC_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RRC(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RRC_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RL(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RL_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RR(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::RR_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SLA(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SLA_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SRA(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SRA_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SWAP(Byte* target_register) {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SWAP_HL() {

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SRL(Byte* target_register) {
        Byte old_LSB = *target_register & BIT0;
        *target_register = (*target_register >> 1);
        this->AF.last = ((*target_register == 0) ? zf : 0) + old_LSB;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SRL_HL() {
        Byte temp = this->ram->ReadByte(this->HL.word);
        Byte old_LSB = temp & BIT0;
        temp = (temp >> 1) & (0 << 7);
        this->AF.last = ((temp == 0) ? zf : 0) + old_LSB;
        this->ram->WriteByte(this->HL.word, temp);

        // Update clocks
        this->M = 4; this->T = 16;
    }

    void Z80::BITTEST(Byte * target_register, unsigned int bit_index) {
        Byte temp = (this->AF.last & cy) ? 0 : cy; // preserve carry
        this->AF.last = h + temp;
        if (*target_register & (1 << bit_index)) {
            this->AF.last += zf;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::BITTEST_HL(unsigned int bit_index) {
        Byte temp = (this->AF.last & cy) ? 0 : cy; // preserve carry
        this->AF.last = h + temp;
        if (this->ram->ReadByte(this->HL.word) & (1 << bit_index)) {
            this->AF.last += zf;
        }

        // Update clocks
        this->M = 4; this->T = 16;
    }

    void Z80::CLEARBIT(Byte * target_register, unsigned int bit_index) {
        Byte temp = *target_register;
        *target_register = temp & (0 << bit_index);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::CLEARBIT_HL(unsigned int bit_index) {
        Byte temp = this->ram->ReadByte(this->HL.word);
        temp &= (0 << bit_index);
        this->ram->WriteByte(this->HL.word, temp);

        // Update clocks
        this->M = 4; this->T = 16;
    }

    void Z80::SETBIT(Byte * target_register, unsigned int bit_index) {
        Byte temp = *target_register;
        *target_register = temp | (1 << bit_index);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    void Z80::SETBIT_HL(unsigned int bit_index) {
        Byte temp = this->ram->ReadByte(this->HL.word);
        temp |= 1 << bit_index;
        this->ram->WriteByte(this->HL.word, temp);

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // CCF: function() { var ci=Z80._r.f&0x10?0:0x10; Z80._r.f=(Z80._r.f&0xEF)+ci; Z80._r.m=1; Z80._r.t=4; }
    void Z80::CCF() {
        Byte temp = (this->AF.last & cy) ? 0 : cy;
        this->AF.last = (this->AF.last & 0x80) + temp;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // SCF: function() { Z80._r.f|=0x10; Z80._r.m=1; Z80._r.t=4; },
    void Z80::SCF() {
        this->AF.last |= cy;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // NOP: function() { Z80._r.m=1; Z80._r.t=4; },
    void Z80::NOP() {
        // Update clocks
        this->M = 1; this->T = 4;
    }

    // HALT: function() { Z80._halt=1; Z80._r.m=1; Z80._r.t=4; },
    void Z80::HALT() {
        this->halt = true;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    void Z80::STOP() {
        this->stop = true;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    void Z80::DI() {
        //this->IME = false;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    void Z80::EI() {
        this->IME = true;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // LDrr_bb: function() { Z80._r.b=Z80._r.b; Z80._r.m=1; Z80._r.t=4; },
    void Z80::LD_R_R(Byte* dest_register, Byte* src_register) {
        *dest_register = *src_register;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // LDrn_b: function() { Z80._r.b=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_R_N(Byte* dest_register) {
        // Get address from immediate value at PC
        *dest_register = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDrHLm_b: function() { Z80._r.b=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_R_HLm(Byte* dest_register) {
        // Write src_register into the address pointed at by HL
        *dest_register = this->ram->ReadByte(this->HL.word);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDHLmr_b: function() { MMU.wb((Z80._r.h<<8)+Z80._r.l,Z80._r.b); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_HLm_R(Byte* src_register) {
        // Write src_register into the address pointed at by HL
        this->ram->WriteByte(this->HL.word, *src_register);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDHLmn: function() { MMU.wb((Z80._r.h<<8)+Z80._r.l, MMU.rb(Z80._r.pc)); Z80._r.pc++; Z80._r.m=3; Z80._r.t=12; },
    void Z80::LD_HLm_N() {
        // Write the immediate value at PC into the address pointed at by HL
        this->ram->WriteByte(this->HL.word, this->ram->ReadByte(this->PC));

        ++this->PC;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // LDABCm: function() { Z80._r.a=MMU.rb((Z80._r.b<<8)+Z80._r.c); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_A_BCm() {
        // Write the value at the address pointed to by BC into A
        this->AF.first = this->ram->ReadByte(this->BC.word);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDADEm: function() { Z80._r.a=MMU.rb((Z80._r.d<<8)+Z80._r.e); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_A_DEm() {
        // Write the value at the address pointed to by DE into A
        this->AF.first = this->ram->ReadByte(this->DE.word);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDAmm: function() { Z80._r.a=MMU.rb(MMU.rw(Z80._r.pc)); Z80._r.pc+=2; Z80._r.m=4; Z80._r.t=16; },
    void Z80::LD_A_NNm() {
        // Write the value at the address described by an immediate value into A
        this->AF.first = this->ram->ReadByte(this->ram->ReadWord(this->PC));

        // Move the PC
        this->PC += 2;

        // Update clocks
        this->M = 4; this->T = 16;
    }


    // LDBCmA: function() { MMU.wb((Z80._r.b<<8)+Z80._r.c, Z80._r.a); Z80._r.m=2; Z80._r.t=8; }
    void Z80::LD_BCm_A() {
        // Write A into the address pointed at by BC
        this->ram->WriteByte(this->BC.word, this->AF.first);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDDEmA: function() { MMU.wb((Z80._r.d<<8)+Z80._r.e, Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_DEm_A() {
        // Write A into the address pointed at by DE
        this->ram->WriteByte(this->DE.word, this->AF.first);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDmmA: function() { MMU.wb(MMU.rw(Z80._r.pc), Z80._r.a); Z80._r.pc+=2; Z80._r.m=4; Z80._r.t=16; },
    void Z80::LD_NNm_A() {
        // Write A into the address described by an immediate value 
        this->ram->WriteByte(this->ram->ReadWord(this->PC), this->AF.first);

        // Move the PC
        this->PC += 2;

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // LDAIOn: function() { Z80._r.a=MMU.rb(0xFF00+MMU.rb(Z80._r.pc)); Z80._r.pc++; Z80._r.m=3; Z80._r.t=12; },
    void Z80::LD_A_IONm() {
        // Get A from value 0xFF + offset from immediate value at PC
        this->AF.first = this->ram->ReadByte(0xFF00 + this->ram->ReadByte(this->PC));

        ++this->PC;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // LDIOnA: function() { MMU.wb(0xFF00+MMU.rb(Z80._r.pc),Z80._r.a); Z80._r.pc++; Z80._r.m=3; Z80._r.t=12; },
    void Z80::LD_IONm_A() {
        // Write A at 0xFF00 + offset from immediate value at PC
        this->ram->WriteByte(0xFF00 + this->ram->ReadByte(this->PC), this->AF.first);

        ++this->PC;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // LDAIOC: function() { Z80._r.a=MMU.rb(0xFF00+Z80._r.c); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_A_IOCm() {
        // Get A from value 0xFF + C
        this->AF.first = this->ram->ReadByte(0xFF00 + this->BC.last);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDIOCA: function() { MMU.wb(0xFF00+Z80._r.c,Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::LD_IOCm_A() {
        // Write A at 0xFF00 + C
        this->ram->WriteByte(0xFF00 + this->BC.last, this->AF.first);

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDHLIA: function() { MMU.wb((Z80._r.h<<8)+Z80._r.l, Z80._r.a); Z80._r.l=(Z80._r.l+1)&255; if(!Z80._r.l) Z80._r.h=(Z80._r.h+1)&255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::LDInc_HLm_A() {
        this->ram->WriteByte(this->HL.word, this->AF.first);

        ++this->HL.word;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDAHLI: function() { Z80._r.a=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.l=(Z80._r.l+1)&255; if(!Z80._r.l) Z80._r.h=(Z80._r.h+1)&255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::LDInc_A_HLm() {
        this->AF.first = this->ram->ReadByte(this->HL.word);

        ++this->HL.word;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDHLDA: function() { MMU.wb((Z80._r.h<<8)+Z80._r.l, Z80._r.a); Z80._r.l=(Z80._r.l-1)&255; if(Z80._r.l==255) Z80._r.h=(Z80._r.h-1)&255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::LDDec_HLm_A() {
        this->ram->WriteByte(this->HL.word, this->AF.first);

        --this->HL.word;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDAHLD: function() { Z80._r.a=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.l=(Z80._r.l-1)&255; if(Z80._r.l==255) Z80._r.h=(Z80._r.h-1)&255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::LDDec_A_HLm() {
        this->AF.first = this->ram->ReadByte(this->HL.word);

        --this->HL.word;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDBCnn: function() { Z80._r.c=MMU.rb(Z80._r.pc); Z80._r.b=MMU.rb(Z80._r.pc+1); Z80._r.pc+=2; Z80._r.m=3; Z80._r.t=12; }
    void Z80::LD_RR_NN(Register* dest_register) {
        // Get address from memory
        dest_register->word = this->ram->ReadWord(this->PC);

        // Move the PC
        this->PC += 2;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // ??
    void Z80::LD_SP_HL() {
        // Write HL into SP
        this->SP.word = this->HL.word;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // LDmmHL: function() { var i=MMU.rw(Z80._r.pc); Z80._r.pc+=2; MMU.ww(i,(Z80._r.h<<8)+Z80._r.l); Z80._r.m=5; Z80._r.t=20; },
    void Z80::LD_NNm_SP() {
        // Get the destination address from the immediate value at PC
        Word temp = this->ram->ReadWord(this->PC);

        // Increment PC
        this->PC += 2;

        // Write SP at temp
        this->ram->WriteWord(temp, this->SP.word);

        // Update clocks
        this->M = 5; this->T = 20;
    }

    // PUSHBC: function() { Z80._r.sp--; MMU.wb(Z80._r.sp,Z80._r.b); Z80._r.sp--; MMU.wb(Z80._r.sp,Z80._r.c); Z80._r.m=3; Z80._r.t=12; }
    void Z80::PUSH_RR(Register* src_register) {
        // Decrement the stack pointer 
        this->SP.word -= 2;

        // Write src_register
        this->ram->WriteWord(this->SP.word, src_register->word);

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // POPBC: function() { Z80._r.c=MMU.rb(Z80._r.sp); Z80._r.sp++; Z80._r.b=MMU.rb(Z80._r.sp); Z80._r.sp++; Z80._r.m=3; Z80._r.t=12; },
    void Z80::POP_RR(Register* dest_register) {
        // Read src_register
        dest_register->word = this->ram->ReadWord(this->SP.word);

        // Increment the stack pointer and read dest_register
        this->SP.word += 2;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // ADDr_b: function() { Z80._r.a+=Z80._r.b; Z80._ops.fz(Z80._r.a); if(Z80._r.a>255) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=1; Z80._r.t=4; },
    void Z80::ADD_A_R(Byte* src_register) {
        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first + *src_register) == 0) {
            this->AF.last |= zf;
        }

        // Check if there is a carry
        if ((this->AF.first + *src_register) > 0xFF) {
            this->AF.last |= cy;
        }

        // Check if there is a half carry
        if (((this->AF.first & 0x0F) + (*src_register & 0x0F)) > 0x0F) {
            this->AF.last |= h;
        }

        // Store the masked first byte in A
        this->AF.first += *src_register;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // ADDn: function() { Z80._r.a+=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._ops.fz(Z80._r.a); if(Z80._r.a>255) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::ADD_A_N() {
        // Get N
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first + temp) == 0) {
            this->AF.last |= zf;
        }

        // Carry
        if ((this->AF.first + temp) > 0xFF) {
            this->AF.last |= cy;
        }

        // Half carry
        if (((this->AF.first & 0x0F) + (temp & 0x0F)) > 0x0F) {
            this->AF.last |= h;
        }

        // Store the masked first byte in A
        this->AF.first += temp;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ADDHL: function() { Z80._r.a+=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._ops.fz(Z80._r.a); if(Z80._r.a>255) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::ADD_A_HLm() {
        // Get HL
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if (!(this->AF.first + (temp & 0xFF))) {
            this->AF.last |= zf;
        }

        // Carry
        if ((this->AF.first + temp) > 0xFF) {
            this->AF.last |= cy;
        }

        // Half carry
        if (((this->AF.first & 0x0F) + (temp & 0x0F)) > 0x0F) {
            this->AF.last |= h;
        }

        // Store the masked first byte in A
        this->AF.first += temp;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ADCr_b: function() { Z80._r.a+=Z80._r.b; Z80._r.a+=(Z80._r.f&0x10)?1:0; Z80._ops.fz(Z80._r.a); if(Z80._r.a>255) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=1; Z80._r.t=4; }
    void Z80::ADC_A_R(Byte* src_register) {
        // Determine the carry flag
        Byte carry = (this->AF.last & cy) ? 1 : 0;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first + *src_register + carry) == 0) {
            this->AF.last |= zf;
        }

        // Carry
        if ((this->AF.first + *src_register + carry) > 0xFF) {
            this->AF.last |= cy;
        }

        // Half carry
        if (((this->AF.first & 0x0F) + (*src_register & 0x0F) + carry) > 0x0F) {
            this->AF.last |= h;
        }

        // Store the masked first byte in A
        this->AF.first = this->AF.first + *src_register + carry;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // ADCn: function() { Z80._r.a+=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._r.a+=(Z80._r.f&0x10)?1:0; Z80._ops.fz(Z80._r.a); if(Z80._r.a>255) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::ADC_A_N() {
        // Get N
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Determine the carry flag
        Byte carry = (this->AF.last & cy) ? 1 : 0;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first + temp + carry) == 0) {
            this->AF.last |= zf;
        }

        // Carry
        if ((this->AF.first + temp + carry) > 0xFF) {
            this->AF.last |= cy;
        }

        // Half carry
        if (((this->AF.first & 0x0F) + (temp & 0x0F) + carry) > 0x0F) {
            this->AF.last |= h;
        }

        // Store the masked first byte in A
        this->AF.first = this->AF.first + temp + carry;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ADCHL: function() { Z80._r.a+=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.a+=(Z80._r.f&0x10)?1:0; Z80._ops.fz(Z80._r.a); if(Z80._r.a>255) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::ADC_A_HLm() {
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Determine the carry flag
        Byte carry = (this->AF.last & cy) ? 1 : 0;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first + temp + carry) == 0) {
            this->AF.last |= zf;
        }

        // Carry
        if ((this->AF.first + temp + carry) > 0xFF) {
            this->AF.last |= cy;
        }

        // Half carry
        if (((this->AF.first & 0x0F) + (temp & 0x0F) + carry) > 0x0F) {
            this->AF.last |= h;
        }

        // Store the masked first byte in A
        this->AF.first = this->AF.first + temp + carry;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // SUBr_b: function() { Z80._r.a-=Z80._r.b; Z80._ops.fz(Z80._r.a,1); if(Z80._r.a<0) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=1; Z80._r.t=4; },
    void Z80::SUB_A_R(Byte* src_register) {
        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first - *src_register) == 0) {
            this->AF.last |= zf;
        }

        // Subtraction
        this->AF.last |= n;

        // Carry
        if (this->AF.first < *src_register) {
            this->AF.last |= cy;
        }

        // Half carry
        if ((this->AF.first & 0x0F) < (*src_register & 0x0F)) {
            this->AF.last |= cy;
        }

        // Store the masked first byte in A
        this->AF.first -= *src_register;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // SUBn: function() { Z80._r.a-=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._ops.fz(Z80._r.a,1); if(Z80._r.a<0) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::SUB_A_N() {
        // Get N
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first - temp) == 0) {
            this->AF.last |= zf;
        }

        // Subtraction
        this->AF.last |= n;

        // Carry
        if (this->AF.first < temp) {
            this->AF.last |= cy;
        }

        // Half carry
        if ((this->AF.first & 0x0F) < (temp & 0x0F)) {
            this->AF.last |= cy;
        }

        // Store the masked first byte in A
        this->AF.first -= temp;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // SUBHL: function() { Z80._r.a-=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._ops.fz(Z80._r.a,1); if(Z80._r.a<0) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::SUB_A_HLm() {
        // Get N
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first - temp) == 0) {
            this->AF.last |= zf;
        }

        // Subtraction
        this->AF.last |= n;

        // Carry
        if (this->AF.first < temp) {
            this->AF.last |= cy;
        }

        // Half carry
        if ((this->AF.first & 0x0F) < (temp & 0x0F)) {
            this->AF.last |= cy;
        }

        // Store the masked first byte in A
        this->AF.first -= temp;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // SBCr_b: function() { Z80._r.a-=Z80._r.b; Z80._r.a-=(Z80._r.f&0x10)?1:0; Z80._ops.fz(Z80._r.a,1); if(Z80._r.a<0) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=1; Z80._r.t=4; },
    void Z80::SBC_A_R(Byte* src_register) {
        Byte carry = (this->AF.last & cy) ? 1 : 0;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first - *src_register - carry) == 0) {
            this->AF.last |= zf;
        }

        // Subtraction
        this->AF.last |= n;

        // Carry
        if (this->AF.first < (*src_register - carry)) {
            this->AF.last |= cy;
        }

        // Half carry
        if ((this->AF.first & 0x0F) < ((*src_register & 0x0F) - carry)) {
            this->AF.last |= cy;
        }

        // Store the masked first byte in A
        this->AF.first -= *src_register - carry;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // SBCn: function() { Z80._r.a-=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._r.a-=(Z80._r.f&0x10)?1:0; Z80._ops.fz(Z80._r.a,1); if(Z80._r.a<0) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::SBC_A_N() {
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        Byte carry = (this->AF.last & cy) ? 1 : 0;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first - temp - carry) == 0) {
            this->AF.last |= zf;
        }

        // Subtraction
        this->AF.last |= n;

        // Carry
        if (this->AF.first < (temp - carry)) {
            this->AF.last |= cy;
        }

        // Half carry
        if ((this->AF.first & 0x0F) < ((temp & 0x0F) - carry)) {
            this->AF.last |= cy;
        }

        // Store the masked first byte in A
        this->AF.first -= temp - carry;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // SBCHL: function() { Z80._r.a-=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.a-=(Z80._r.f&0x10)?1:0; Z80._ops.fz(Z80._r.a,1); if(Z80._r.a<0) Z80._r.f|=0x10; Z80._r.a&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::SBC_A_HLm() {
        Byte temp = this->ram->ReadByte(this->HL.word);

        Byte carry = (this->AF.last & cy) ? 1 : 0;

        // Reset the flags
        this->AF.last = 0;

        // Zero
        if ((this->AF.first - temp - carry) == 0) {
            this->AF.last |= zf;
        }

        // Subtraction
        this->AF.last |= n;

        // Carry
        if (this->AF.first < (temp - carry)) {
            this->AF.last |= cy;
        }

        // Half carry
        if ((this->AF.first & 0x0F) < ((temp & 0x0F) - carry)) {
            this->AF.last |= cy;
        }

        // Store the masked first byte in A
        this->AF.first -= temp - carry;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ANDr_b: function() { Z80._r.a&=Z80._r.b; Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=1; Z80._r.t=4; },
    void Z80::AND_A_R(Byte* src_register) {
        // Reset the flags
        this->AF.last = h;

        // Check if the operation resulted in 0
        if ((this->AF.first & *src_register) == 0) {
            this->AF.last |= zf;
        }

        // Store the masked first byte in A
        this->AF.first &= *src_register;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // ANDn: function() { Z80._r.a&=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=2; Z80._r.t=8; }
    void Z80::AND_A_N() {
        // Binary AND A and src_register
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = h;

        // Check if the operation resulted in 0
        if ((this->AF.first & temp) == 0) {
            this->AF.last |= zf;
        }

        // Store the masked first byte in A
        this->AF.first &= temp;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ANDHL: function() { Z80._r.a&=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::AND_A_HLm() {
        // Binary AND A and src_register
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Reset the flags
        this->AF.last = h;

        // Check if the operation resulted in 0
        if ((this->AF.first & temp) == 0) {
            this->AF.last |= zf;
        }

        // Store the masked first byte in A
        this->AF.first &= temp;

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // XORr_b: function() { Z80._r.a^=Z80._r.b; Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=1; Z80._r.t=4; },
    void Z80::XOR_A_R(Byte* src_register) {
        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first ^= *src_register) == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // XORn: function() { Z80._r.a^=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::XOR_A_N() {
        // Binary XOR A and the value at PC
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first ^= temp) == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // XORHL: function() { Z80._r.a^=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::XOR_A_HLm() {
        // Binary XOR A and the value at PC
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first ^= temp) == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ORr_b: function() { Z80._r.a|=Z80._r.b; Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=1; Z80._r.t=4; },
    void Z80::OR_A_R(Byte* src_register) {
        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first |= *src_register) == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // ORn: function() { Z80._r.a|=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::OR_A_N() {
        // Binary XOR A and the value at PC
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first |= temp) == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // ORHL: function() { Z80._r.a|=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._r.a&=255; Z80._ops.fz(Z80._r.a); Z80._r.m=2; Z80._r.t=8; },
    void Z80::OR_A_HLm() {
        // Binary XOR A and the value at PC
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Reset the flags
        this->AF.last = 0;

        // Check if the operation resulted in 0
        if ((this->AF.first |= temp) == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // CPr_b: function() { var i=Z80._r.a; i-=Z80._r.b; Z80._ops.fz(i,1); if(i<0) Z80._r.f|=0x10; i&=255; Z80._r.m=1; Z80._r.t=4; },
    void Z80::CP_A_R(Byte* src_register) {
        // Reset the flags
        this->AF.last = n;

        // Check if the operation resulted in 0
        if (this->AF.first == *src_register) {
            this->AF.last |= zf;
        }

        // Check if there is a carry
        if (this->AF.first < *src_register) {
            this->AF.last |= cy;
        }

        if ((this->AF.first & 0x0F) < (*src_register & 0x0F)) {
            this->AF.last |= h;
        }

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // CPn: function() { var i=Z80._r.a; i-=MMU.rb(Z80._r.pc); Z80._r.pc++; Z80._ops.fz(i,1); if(i<0) Z80._r.f|=0x10; i&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::CP_A_N() {
        // Store A in a temp variable
        Byte temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = n;

        // Subtract the value at PC
        if (temp == this->AF.first) {
            this->AF.last |= zf;
        }

        // Check if there is a carry
        if (this->AF.first < temp) {
            this->AF.last |= cy;
        }

        if ((this->AF.first & 0x0F) < (temp & 0x0F)) {
            this->AF.last |= h;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // CPHL: function() { var i=Z80._r.a; i-=MMU.rb((Z80._r.h<<8)+Z80._r.l); Z80._ops.fz(i,1); if(i<0) Z80._r.f|=0x10; i&=255; Z80._r.m=2; Z80._r.t=8; },
    void Z80::CP_A_HLm() {
        // Store A in a temp variable
        Byte temp = this->ram->ReadByte(this->HL.word);

        // Reset the flags
        this->AF.last = n;

        // Subtract the value at PC
        if (temp == this->AF.first) {
            this->AF.last |= zf;
        }

        // Check if there is a carry
        if (this->AF.first < temp) {
            this->AF.last |= cy;
        }

        if ((this->AF.first & 0x0F) < (temp & 0x0F)) {
            this->AF.last |= h;
        }

        // Update clocks
        this->M = 2; this->T = 8;
    }

    // INCr_b: function() { Z80._r.b++; Z80._r.b&=255; Z80._ops.fz(Z80._r.b); Z80._r.m=1; Z80._r.t=4; },
    void Z80::INC_R(Byte* sd_register) {
        // Reset the flags
        this->AF.last &= cy;

        if ((*sd_register & 0x0F + 1) == 0x0F) {
            this->AF.last |= h;
        }

        // Check if the operation resulted in 0
        if (++*sd_register == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // INCHLm: function() { var i=MMU.rb((Z80._r.h<<8)+Z80._r.l)+1; i&=255; MMU.wb((Z80._r.h<<8)+Z80._r.l,i); Z80._ops.fz(i); Z80._r.m=3; Z80._r.t=12; },
    void Z80::INC_HLm() {
        // Increment sd_register and store it in a temp
        Byte temp = this->ram->ReadByte(this->HL.word) + 1;

        // Reset the flags
        this->AF.last &= cy;

        if ((temp & 0x0F + 1) == 0x0F) {
            this->AF.last |= h;
        }

        // Check if the operation resulted in 0
        if (++temp == 0) {
            this->AF.last |= zf;
        }
        this->ram->WriteByte(this->HL.word, temp);

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // DECr_b: function() { Z80._r.b--; Z80._r.b&=255; Z80._ops.fz(Z80._r.b); Z80._r.m=1; Z80._r.t=4; },
    void Z80::DEC_R(Byte* sd_register) {
        // Reset the flags
        this->AF.last &= cy;

        this->AF.last |= n;

        if ((*sd_register & 0x0F - 1) == 0x00) {
            this->AF.last |= h;
        }

        // Check if the operation resulted in 0
        if (--*sd_register == 0) {
            this->AF.last |= zf;
        }

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // DECHLm: function() { var i=MMU.rb((Z80._r.h<<8)+Z80._r.l)-1; i&=255; MMU.wb((Z80._r.h<<8)+Z80._r.l,i); Z80._ops.fz(i); Z80._r.m=3; Z80._r.t=12; },
    void Z80::DEC_HLm() {
        // Increment sd_register and store it in a temp
        Byte temp = this->ram->ReadByte(this->HL.word) - 1;

        // Reset the flags
        this->AF.last &= cy;

        this->AF.last |= n;

        if ((temp & 0x0F - 1) == 0x00) {
            this->AF.last |= h;
        }

        // Check if the operation resulted in 0
        if (--temp == 0) {
            this->AF.last |= zf;
        }

        // Store the masked first byte in sd_register
        this->ram->WriteByte(this->HL.word, temp);

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // XX
    void Z80::DAA() {

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // CPL: function() { Z80._r.a = (~Z80._r.a)&255; Z80._ops.fz(Z80._r.a,1); Z80._r.m=1; Z80._r.t=4; },
    void Z80::CPL() {
        this->AF.first ^= 0xFF;

        this->AF.last |= n | h;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // ADDHLBC: function() { var hl=(Z80._r.h<<8)+Z80._r.l; hl+=(Z80._r.b<<8)+Z80._r.c; if(hl>65535) Z80._r.f|=0x10; else Z80._r.f&=0xEF; Z80._r.h=(hl>>8)&255; Z80._r.l=hl&255; Z80._r.m=3; Z80._r.t=12; }
    void Z80::ADD_HL_RR(Register* src_register) {
        // Reset the flags
        this->AF.last &= zf;

        // Check if there was a half carry
        if (((this->HL.word & 0xFFF) + (src_register->word & 0xFFF)) > 0xFFF) {
            this->AF.last |= h;
        }

        // Set the carry flag
        if ((this->HL.word + src_register->word) > 0xFFFF) {
            this->AF.last |= cy;
        }

        // Store the masked first word in HL
        this->HL.word += src_register->word;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // ADDSPn: function() { var i=MMU.rb(Z80._r.pc); if(i>127) i=-((~i+1)&255); Z80._r.pc++; Z80._r.sp+=i; Z80._r.m=4; Z80._r.t=16; },
    void Z80::ADD_SP_dd() {
        // Get D
        char temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = 0;

        // Check if there was a half carry
        if (((this->SP.word & 0xFFF) + (temp & 0xFFF)) > 0xFFF) {
            this->AF.last |= h;
        }

        // Set the carry flag
        if (((int)this->SP.word + temp) > 0xFFFF) {
            this->AF.last |= cy;
        }

        // Add d to SP
        this->SP.word += temp;

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // INCBC: function() { Z80._r.c=(Z80._r.c+1)&255; if(!Z80._r.c) Z80._r.b=(Z80._r.b+1)&255; Z80._r.m=1; Z80._r.t=4; },
    void Z80::INC_RR(Register* sd_register) {
        ++sd_register->word;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // DECBC: function() { Z80._r.c=(Z80._r.c-1)&255; if(Z80._r.c==255) Z80._r.b=(Z80._r.b-1)&255; Z80._r.m=1; Z80._r.t=4; },
    void Z80::DEC_RR(Register* sd_register) {
        --sd_register->word;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    //LDHLSPn: function() { var i=MMU.rb(Z80._r.pc); if(i>127) i=-((~i+1)&255); Z80._r.pc++; i+=Z80._r.sp; Z80._r.h=(i>>8)&255; Z80._r.l=i&255; Z80._r.m=3; Z80._r.t=12; },
    void Z80::LD_HL_SPdd() {
        // Get d
        char temp = this->ram->ReadByte(this->PC);

        ++this->PC;

        // Reset the flags
        this->AF.last = 0;

        // Check if there was a half carry
        if (((this->HL.word & 0x0F) + (temp & 0x0F) + (this->SP.word & 0x0F)) > 0x0F) {
            this->AF.last |= h;
        }

        // Set the carry flag
        if (((int)this->SP.word + temp + (int)this->HL.word) > 0xFF) {
            this->AF.last |= cy;
        }

        // Store SP in HL
        this->HL.word = this->SP.word + temp;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // RLCA: function() { var ci=Z80._r.a&0x80?1:0; var co=Z80._r.a&0x80?0x10:0; Z80._r.a=(Z80._r.a<<1)+ci; Z80._r.a&=255; Z80._r.f=(Z80._r.f&0xEF)+co; Z80._r.m=1; Z80._r.t=4; },
    void Z80::RLCA() {
        Byte ci = (this->AF.first & BIT7) ? 1 : 0;
        Byte co = (this->AF.first & BIT7) ? 0x10 : 0;

        this->AF.first = ((this->AF.first << 1) + ci) & 0xFF;

        this->AF.last = co;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // RLA: function() { var ci=Z80._r.f&0x10?1:0; var co=Z80._r.a&0x80?0x10:0; Z80._r.a=(Z80._r.a<<1)+ci; Z80._r.a&=255; Z80._r.f=(Z80._r.f&0xEF)+co; Z80._r.m=1; Z80._r.t=4; },
    void Z80::RLA() {
        Byte ci = (this->AF.last & BIT4) ? 1 : 0;
        Byte co = (this->AF.first & BIT7) ? 0x10 : 0;

        this->AF.first = ((this->AF.first << 1) + ci) & 0xFF;

        this->AF.last = co;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // RRCA: function() { var ci=Z80._r.a&1?0x80:0; var co=Z80._r.a&1?0x10:0; Z80._r.a=(Z80._r.a>>1)+ci; Z80._r.a&=255; Z80._r.f=(Z80._r.f&0xEF)+co; Z80._r.m=1; Z80._r.t=4; },
    void Z80::RRCA() {
        Byte ci = (this->AF.first & 1) ? 0x80 : 0;
        Byte co = (this->AF.first & 1) ? 0x10 : 0;

        this->AF.first = ((this->AF.first >> 1) + ci) & 0xFF;

        this->AF.last = co;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // RRA: function() { var ci=Z80._r.f&0x10?0x80:0; var co=Z80._r.a&1?0x10:0; Z80._r.a=(Z80._r.a>>1)+ci; Z80._r.a&=255; Z80._r.f=(Z80._r.f&0xEF)+co; Z80._r.m=1; Z80._r.t=4; },
    void Z80::RRA() {
        Byte ci = (this->AF.last & BIT4) ? 0x80 : 0;
        Byte co = (this->AF.first & 1) ? 0x10 : 0;

        this->AF.first = ((this->AF.first >> 1) + ci) & 0xFF;

        this->AF.last = co;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // JPnn: function() { Z80._r.pc = MMU.rw(Z80._r.pc); Z80._r.m=3; Z80._r.t=12; },
    void Z80::JP_NN() {
        this->PC = this->ram->ReadWord(this->PC);

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // JPHL: function() { Z80._r.pc=Z80._r.hl; Z80._r.m=1; Z80._r.t=4; },
    void Z80::JP_HL() {
        this->PC = this->HL.word;

        // Update clocks
        this->M = 1; this->T = 4;
    }

    // JPZnn: function()  { Z80._r.m=3; Z80._r.t=12; if((Z80._r.f&0x80)==0x80) { Z80._r.pc=MMU.rw(Z80._r.pc); Z80._r.m++; Z80._r.t+=4; } else Z80._r.pc+=2; },
    void Z80::JP_F_NN(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == flag) {
            this->PC = this->ram->ReadWord(this->PC);

            // Update clocks
            this->M = 4; this->T = 16;
        }
        else {
            this->PC += 2;
            this->M = 3; this->T = 12;
        }
    }

    // JPNZnn: function() { Z80._r.m=3; Z80._r.t=12; if((Z80._r.f&0x80)==0x00) { Z80._r.pc=MMU.rw(Z80._r.pc); Z80._r.m++; Z80._r.t+=4; } else Z80._r.pc+=2; },
    void Z80::JP_NOTF_NN(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == 0) {
            this->PC = this->ram->ReadWord(this->PC);

            // Update clocks
            this->M = 4; this->T = 16;
        }
        else {
            this->PC += 2;
            // Update clocks
            this->M = 3; this->T = 12;
        }
    }

    // JRn: function() { var i=MMU.rb(Z80._r.pc); if(i>127) i=-((~i+1)&255); Z80._r.pc++; Z80._r.m=2; Z80._r.t=8; Z80._r.pc+=i; Z80._r.m++; Z80._r.t+=4; },
    void Z80::JR_PCdd() {
        this->PC += (char)this->ram->ReadByte(this->PC);

        ++this->PC;

        // Update clocks
        this->M = 3; this->T = 12;
    }

    // JRZn: function()  { var i=MMU.rb(Z80._r.pc); if(i>127) i=-((~i+1)&255); Z80._r.pc++; Z80._r.m=2; Z80._r.t=8; if((Z80._r.f&0x80)==0x80) { Z80._r.pc+=i; Z80._r.m++; Z80._r.t+=4; } },
    void Z80::JR_F_PCdd(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == flag) {
            this->PC += (char)this->ram->ReadByte(this->PC);

            ++this->PC;

            // Update clocks
            this->M = 3; this->T = 12;
        }
        else {
            ++this->PC;

            // Update clocks
            this->M = 2; this->T = 8;
        }
    }

    // JRNZn: function() { var i=MMU.rb(Z80._r.pc); if(i>127) i=-((~i+1)&255); Z80._r.pc++; Z80._r.m=2; Z80._r.t=8; if((Z80._r.f&0x80)==0x00) { Z80._r.pc+=i; Z80._r.m++; Z80._r.t+=4; } },
    void Z80::JR_NOTF_PCdd(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == 0) {
            this->PC += (char)this->ram->ReadByte(this->PC);

            ++this->PC;

            // Update clocks
            this->M = 3; this->T = 12;
        }
        else {
            ++this->PC;

            // Update clocks
            this->M = 2; this->T = 8;
        }
    }

    // CALLnn: function() { Z80._r.sp-=2; MMU.ww(Z80._r.sp,Z80._r.pc+2); Z80._r.pc=MMU.rw(Z80._r.pc); Z80._r.m=5; Z80._r.t=20; },
    void Z80::CALL_NN() {
        this->SP.word -= 2;

        this->ram->WriteWord(this->SP.word, this->PC + 2);

        this->PC = this->ram->ReadWord(this->PC);

        // Update clocks
        this->M = 6; this->T = 24;
    }

    // CALLZnn: function() { Z80._r.m=3; Z80._r.t=12; if((Z80._r.f&0x80)==0x80) { Z80._r.sp-=2; MMU.ww(Z80._r.sp,Z80._r.pc+2); Z80._r.pc=MMU.rw(Z80._r.pc); Z80._r.m+=2; Z80._r.t+=8; } else Z80._r.pc+=2; },
    void Z80::CALL_F_NN(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == flag) {
            this->SP.word -= 2;

            this->ram->WriteWord(this->SP.word, this->PC + 2);

            this->PC = this->ram->ReadWord(this->PC);

            // Update clocks
            this->M = 6; this->T = 24;
        }
        else {
            this->PC += 2;

            // Update clocks
            this->M = 3; this->T = 12;
        }
    }

    // CALLNZnn: function() { Z80._r.m=3; Z80._r.t=12; if((Z80._r.f&0x80)==0x00) { Z80._r.sp-=2; MMU.ww(Z80._r.sp,Z80._r.pc+2); Z80._r.pc=MMU.rw(Z80._r.pc); Z80._r.m+=2; Z80._r.t+=8; } else Z80._r.pc+=2; },
    void Z80::CALL_NOTF_NN(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == 0) {
            this->SP.word -= 2;

            this->ram->WriteWord(this->SP.word, this->PC + 2);

            this->PC = this->ram->ReadWord(this->PC);

            // Update clocks
            this->M = 6; this->T = 24;
        }
        else {
            this->PC += 2;
            // Update clocks
            this->M = 3; this->T = 12;
        }
    }

    // RET: function() { Z80._r.pc=MMU.rw(Z80._r.sp); Z80._r.sp+=2; Z80._r.m=3; Z80._r.t=12; },
    void Z80::RET() {
        // Restore the PC from the address in SP
        this->PC = this->ram->ReadWord(this->SP.word);

        // Move the stack down 2
        this->SP.word += 2;

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // RETZ: function() { Z80._r.m=1; Z80._r.t=4; if((Z80._r.f&0x80)==0x80) { Z80._r.pc=MMU.rw(Z80._r.sp); Z80._r.sp+=2; Z80._r.m+=2; Z80._r.t+=8; } },
    void Z80::RET_F(FLAGS_REGISTER flag) {
        if ((this->AF.last & flag) == flag) {
            // Restore the PC from the address in SP
            this->PC = this->ram->ReadWord(this->SP.word);

            // Move the stack down 2
            this->SP.word += 2;

            // Update clocks
            this->M = 5; this->T = 20;
        }
        else {
            // Update clocks
            this->M = 2; this->T = 8;
        }
    }

    // RETNZ: function() { Z80._r.m=1; Z80._r.t=4; if((Z80._r.f&0x80)==0x00) { Z80._r.pc=MMU.rw(Z80._r.sp); Z80._r.sp+=2; Z80._r.m+=2; Z80._r.t+=8; } },
    void Z80::RET_NOTF(FLAGS_REGISTER flag) {

        if ((this->AF.last & flag) == 0) {
            // Restore the PC from the address in SP
            this->PC = this->ram->ReadWord(this->SP.word);


            // Move the stack down 2
            this->SP.word += 2;

            // Update clocks
            this->M = 5; this->T = 20;
        }
        else {
            // Update clocks
            this->M = 2; this->T = 8;
        }
    }

    // RETI: function() { Z80._r.ime=1; Z80._r.pc=MMU.rw(Z80._r.sp); Z80._r.sp+=2; Z80._r.m=3; Z80._r.t=12; },
    void Z80::RETI() {
        // Enable all interrupts
        this->IME = true;

        // Restore the PC from the address in SP
        this->PC = this->ram->ReadWord(this->SP.word);

        // Move the stack down 2
        this->SP.word += 2;

        // Update clocks
        this->M = 4; this->T = 16;
    }

    // RST00: function() { Z80._r.sp-=2; MMU.ww(Z80._r.sp,Z80._r.pc); Z80._r.pc=0x00; Z80._r.m=3; Z80._r.t=12; },
    void Z80::RST_N(Word nextPC) {
        // Move the stack up 2
        this->SP.word -= 2;

        // Store the current PC at address in SP
        this->ram->WriteWord(this->SP.word, this->PC);

        // Set the PC to the provided value
        this->PC = nextPC;

        // Update clocks
        this->M = 3; this->T = 12;
    }

}
