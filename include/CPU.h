#pragma once
#include "Binary.h"

#include "../include/Memory.h"

// Reference material most found http://nocash.emubase.de/pandocs.htm#cpuinstructionset

namespace Processor {
	enum FLAGS_REGISTER {
					// 0-3 Unused
		cy	= BIT4, // Carry
		h	= BIT5, // Half Carry
		n	= BIT6, // Add/Sub
		zf	= BIT7, // Zero
	};

	union Register {
		Register() : word(0) { }
		Word word;
		struct {
			Byte last;
			Byte first;
		};
	};

	enum INTERRUPTS {
		VBLANK = BIT0,
		LCDC_STATUS = BIT1,
		TIMER_OVERFLOW = BIT2,
		SERIAL_LINK = BIT3,
		JOYPAD_LINK = BIT4
	};

	class Z80 {
	public:
		//typedef void (Z80::*OP_FUNC)(void); // Function pointer

		Z80();
		~Z80();

		// Set the MMU
		void SetMMU(Memory::MMU* r);

		// Set PC
		void SetPC( Word address );

		// Get PC
		Word GetPC();

		// Get the last OP's T count
		int GetT();

        int GetTotalT();
		
		// Get the next OP at PC, increment PC, and execute the OP.
		bool DoNextOp();
        void DoInterrupts();

        void DoCBOp();

        /************************************************************************/
        /* CB Opcodes start                                                    */
        /************************************************************************/

        void RLC(Byte* target_register);
        void RLC_HL();
        void RRC(Byte* target_register);
        void RRC_HL();
        void RL(Byte* target_register);
        void RL_HL();
        void RR(Byte* target_register);
        void RR_HL();
        void SLA(Byte* target_register);
        void SLA_HL();
        void SRA(Byte* target_register);
        void SRA_HL();
        void SWAP(Byte* target_register);
        void SWAP_HL();
        void SRL(Byte* target_register);
        void SRL_HL();
        void BITTEST(Byte* target_register, unsigned int bit_index);
        void BITTEST_HL(unsigned int bit_index);
        void CLEARBIT(Byte* target_register, unsigned int bit_index);
        void CLEARBIT_HL(unsigned int bit_index);
        void SETBIT(Byte* target_register, unsigned int bit_index);
        void SETBIT_HL(unsigned int bit_index);

        /************************************************************************/
        /* CB Opcodes end                                                    */
        /************************************************************************/

		/************************************************************************/
		/* CPU Control start                                                    */
		/************************************************************************/

		// Carry flag complement (CCF)
		 void CCF();

		// Set carry flag to 1 (SCF)
		 void SCF();

		// No Operation
		 void NOP();

		// Halt
		 void HALT();

		// Stop
		 void STOP();

		// Disable interrupts (DI)
		 void DI();

		// Enable interrupts (EI)
		 void EI();

		/************************************************************************/
		/* CPU Control end                                                    */
		/************************************************************************/

		/************************************************************************/
		/* 8-bit Load start                                                     */
		/************************************************************************/

		// Copy a byte from a register into another registers (LD r, r)
		 void LD_R_R(Byte* dest_register, Byte* src_register);

		// Copy a byte from an immediate value into a register (LD r, n)
		 void LD_R_N(Byte* dest_register);

		// Copy a byte from a memory address in HL into a register (LD r, (HL))
		 void LD_R_HLm(Byte* dest_register);

		// Copy a byte from a register into a memory address in HL (LD (HL), r)
		 void LD_HLm_R(Byte* src_register);

		// Copy a byte from an immediate value into a memory address in HL (LD (HL), n)
		 void LD_HLm_N();

		// Copy a byte from a memory address in BC into A (LD A, (BC))
		 void LD_A_BCm();

		// Copy a byte from a memory address in DE into A (LD A, (DE))
		 void LD_A_DEm();

		// Copy a byte from a memory address in an immediate value into A (LD A, (DE))
		 void LD_A_NNm();

		// Copy a byte from A into a memory address in BC (LD (BC), A)
		 void LD_BCm_A();

		// Copy a byte from A into a memory address in DE (LD (DE), A)
		 void LD_DEm_A();

		// Copy a byte from A into a memory address in an immediate value (LD (DE), A)
		 void LD_NNm_A();

		// Copy a byte from IO memory at FF00+N into A (LD A, (FF00+n))
		 void LD_A_IONm();

		// Copy a byte from A into IO memory at FF00+N (LD (FF00+n), A)
		 void LD_IONm_A();

		// Copy a byte from IO memory at FF00+C into A (LD A, (FF00+C))
		 void LD_A_IOCm();

		// Copy a byte from A into IO memory at FF00+C (LD (FF00+C), A)
		 void LD_IOCm_A();

		// Copy a byte from A into a memory address in HL, increment HL (LDI (HL), A)
		 void LDInc_HLm_A();

		// Copy a byte from a memory address in HL into A, increment HL (LDI A, (HL))
		 void LDInc_A_HLm();

		// Copy a byte from A into a memory address in HL, decrement HL (LDD (HL), A)
		 void LDDec_HLm_A();

		// Copy a byte from a memory address in HL into A, decrement HL (LDD A, (HL))
		 void LDDec_A_HLm();

		/************************************************************************/
		/* 8-bit Load end                                                       */
		/************************************************************************/

		/************************************************************************/
		/* 16-bit Load start                                                    */
		/************************************************************************/

		// Read a word from an immediate value into a register (LD rr, nn) (rr may be BC,DE,HL or SP)
		 void LD_RR_NN(Register* dest_register);

		// Write HL into SP (LD SP, HL)
		 void LD_SP_HL();

		// Write SP into a memory address in an immediate value (LD (nn), SP)
		 void LD_NNm_SP();

		// Decrement the SP by 2 and push a register onto the stack (push rr) (rr may be BC,DE,HL,AF)
		 void PUSH_RR(Register* src_register);

		// Pop a register from the stack and increment the SP by 2 (pop rr) (rr may be BC,DE,HL,AF)
		 void POP_RR(Register* dest_register);

		/************************************************************************/
		/* 16-bit Load end                                                      */
		/************************************************************************/

		/************************************************************************/
		/* 8-bit ALU start                                                      */
		/************************************************************************/

		// Add a register to A and store it in A (ADD A, r)
		 void ADD_A_R(Byte* src_register);

		// Add an immediate value to A and store it in A (ADD A, n)
		 void ADD_A_N();

		// Add the value at memory address in HL to A and store it in A (ADD A, (HL))
		 void ADD_A_HLm();

		// Add a register and the carry flag to A and store it in A (ADC A, r)
		 void ADC_A_R(Byte* src_register);

		// Add an immediate value and the carry flag to A and store it in A (ADC A, n)
		 void ADC_A_N();

		// Add the value at memory address in HL and the carry flag to A and store it in A (ADC A, (HL))
		 void ADC_A_HLm();

		// Subtract a register from A and store it in A (SUB r)
		 void SUB_A_R(Byte* src_register);

		// Subtract an immediate value from A and store it in A (SUB n)
		 void SUB_A_N();

		// Subtract the value at memory address in HL from A and store it in A (SUB (HL))
		 void SUB_A_HLm();

		// Subtract a register and the carry flag from A and store it in A (SBC A, r)
		 void SBC_A_R(Byte* src_register);

		// Subtract an immediate value and the carry flag from A and store it in A (SBC A, n)
		 void SBC_A_N();

		// Subtract the value at memory address in HL and the carry flag from A and store it in A (SBC A, (HL))
		 void SBC_A_HLm();

		// Binary AND a register and A (AND r)
		 void AND_A_R(Byte* src_register);

		// Binary AND an immediate value and A (AND n)
		 void AND_A_N();

		// Binary AND the value at memory address in HL and A (AND (HL))
		 void AND_A_HLm();

		// Binary XOR a register and A (XOR r)
		 void XOR_A_R(Byte* src_register);

		// Binary XOR an immediate value and A (XOR n)
		 void XOR_A_N();

		// Binary XOR the value at memory address in HL and A (XOR (HL))
		 void XOR_A_HLm();

		// Binary OR a register and A (OR r)
		 void OR_A_R(Byte* src_register);

		// Binary OR an immediate value and A (OR n)
		 void OR_A_N();

		// Binary OR the value at memory address in HL and A (OR (HL)
		 void OR_A_HLm();

		// Binary compare a register and A (CMP r)
		 void CP_A_R(Byte* src_register);

		// Binary compare an immediate value and A (CMP n)
		 void CP_A_N();

		// Binary compare the value at memory address in HL and A (CMP (HL))
		 void CP_A_HLm();

		// Increment sd_register and store it in sd_register (INC r)
		 void INC_R(Byte* sd_register);

		// Increment the value at memory address in HL and store it at memory address in HL (INC r)
		 void INC_HLm();

		// Decrement sd_register and store it in sd_register (INC r)
		 void DEC_R(Byte* sd_register);

		// Decrement the value at memory address in HL and store it at memory address in HL (INC r)
		 void DEC_HLm();

		// Decimal adjust accumulator (DAA)
		 void DAA();

		// Store the binary complement of A in A (CPL)
		 void CPL();

		/************************************************************************/
		/* 8-bit ALU end                                                        */
		/************************************************************************/

		/************************************************************************/
		/* 16-bit ALU start                                                     */
		/************************************************************************/

		// Add a register to HL and store it in HL (ADD HL, RR)
		 void ADD_HL_RR(Register* src_register);

		// Add a signed value to SP and store it in SP (ADD SP, dd)
		 void ADD_SP_dd();

		// Increment a register and store in to the same one (INC RR)
		 void INC_RR(Register* sd_register);

		// Decrement a register and store in to the same one (DEC RR)
		 void DEC_RR(Register* sd_register);

		// Store the value of SP plus a signed value into HL (LD HL, SP+dd)
		 void LD_HL_SPdd();

		/************************************************************************/
		/* 16-bit ALU end                                                       */
		/************************************************************************/

		/************************************************************************/
		/* Rotate and Shift start                                               */
		/************************************************************************/
		
		 void RLCA();

		 void RLA();

		 void RRCA();

		 void RRA();

		/************************************************************************/
		/* Rotate and Shift end                                                 */
		/************************************************************************/

		/************************************************************************/
		/* Jump and Call start                                                  */
		/************************************************************************/

		// Jump PC to memory address in immediate value (JP nn)
		 void JP_NN();

		// Jump PC to memory address in HL (JP HL)
		 void JP_HL();

		// Jump PC to memory address in immediate value if flag is set (JP f, nn)
		 void JP_F_NN(FLAGS_REGISTER flag);

		// Jump PC to memory address in immediate value if flag is not set (JP f, nn)
		 void JP_NOTF_NN(FLAGS_REGISTER flag);

		 void JR_PCdd();

		 void JR_F_PCdd(FLAGS_REGISTER flag);

		 void JR_NOTF_PCdd(FLAGS_REGISTER flag);

		// Call PC to memory address in immediate value (CALL nn)
		 void CALL_NN();

		// Call PC to memory address in immediate value if flag is set (CALL f, nn)
		 void CALL_F_NN(FLAGS_REGISTER flag);

		// Call PC to memory address in immediate value if flag is not set (CALL f, nn)
		 void CALL_NOTF_NN(FLAGS_REGISTER flag);

		// Return (RET)
		 void RET();

		// Returns if the flag is set (RET f)
		 void RET_F(FLAGS_REGISTER flag);

		// Returns if the flag is not set (RET f)
		 void RET_NOTF(FLAGS_REGISTER flag);

		// Enable all interrupts and return (RETI)
		 void RETI();

		// Reset and move PC to nextPC (RST n)
		 void RST_N(Word nextPC);
		/************************************************************************/
		/* Jump and Call end                                                    */
		/************************************************************************/

	private:
		Register AF, BC, DE, HL, SP; // 16-bit 2-part general registers. We are using shorts to allow carry checks
		Word PC; // 16-bit special registers
		int M, T; // Clocks
		int total_M, total_T; // Total execution time
		bool IME; // Interrupt Master Enable
		bool halt; // If processing is halted
		bool stop; // If stopped

		Memory::MMU* ram;

		unsigned int numInstructions;
	};
}