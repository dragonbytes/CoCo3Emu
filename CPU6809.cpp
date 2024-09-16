#include "GimeBus.h"
#include "CPU6809.h"

// cpu6809 Constructor
Cpu6809::Cpu6809()
{
	waitingForNextOp = false;
	cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
	cpuHardwareHalt = false;
	cpuHaltAsserted = false;
	nextOpWaitCounter = 0;
	cpuCyclesTotal = 0;
	curOpCode = 0;

	cpuReg.CC.E = true;

	//fopen_s(&traceListFile, "D:\\Temp\\coco3emu_trace.lst", "wb");
}

uint8_t Cpu6809::readByteAtCurPC()
{
	// Automatically increments the PC register
	uint8_t readByte = gimeBus->readMemoryByte(cpuReg.PC);
	cpuReg.PC++;
	return readByte;
}

uint16_t Cpu6809::readWordAtCurPC()
{
	// Automatically advances the PC register by 2 bytes
	uint16_t readWord = gimeBus->readMemoryWord(cpuReg.PC);
	cpuReg.PC += 2;
	return readWord;
}

void Cpu6809::cpuClockTick()
{
	#define DEBUGGER_ENABLED 1

	if (nextOpWaitCounter != 0)
		nextOpWaitCounter--;
	else
	{
		if (cpuHardwareHalt)
			return;
		else if (cpuHaltAsserted)
		{
			cpuHardwareHalt = true;
			cpuHaltAsserted = false;
		}

		// Check for asserted interrupt signals
		if (assertedInterrupts[INT_NMI])
		{
			if (cpuSoftHalt != CPU_SOFTWARE_HALT_CWAI)
			{
				cpuReg.CC.E = true;
				operandByte = 0xFF;			// 0xFF = every bit set such that all registers are pushed to hardware stack
				pushToStack(&cpuReg.S);
				cpuReg.CC.F = true;
				cpuReg.CC.I = true;
			}
			cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
			cpuReg.PC = gimeBus->readMemoryWord(VECTOR_NMI);
			// Since NMI interrupts are edge triggered, the Disk Controller (presumably) only sends a brief pulse to trigger the interrupt, and then it immediately returns to normal
			assertedInterrupts[INT_NMI] &= ~INT_ASSERT_MASK_NMI;
		}
		else if (assertedInterrupts[INT_FIRQ])
		{
			if (!cpuReg.CC.F)
			{
				if (cpuSoftHalt != CPU_SOFTWARE_HALT_CWAI)
				{
					cpuReg.CC.E = false;
					operandByte = MASK_PC | MASK_CC;
					pushToStack(&cpuReg.S);
					cpuReg.CC.F = true;
					cpuReg.CC.I = true;
				}
				cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
				cpuReg.PC = gimeBus->readMemoryWord(VECTOR_FIRQ);
			}
			else if (cpuSoftHalt == CPU_SOFTWARE_HALT_SYNC)
				cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
		}
		else if (assertedInterrupts[INT_IRQ])
		{
			if (!cpuReg.CC.I)
			{
				if (cpuSoftHalt != CPU_SOFTWARE_HALT_CWAI)
				{
					cpuReg.CC.E = true;
					operandByte = 0xFF;		// 0xFF = every bit set such that all registers are pushed to hardware stack
					pushToStack(&cpuReg.S);
					cpuReg.CC.I = true;
				}
				cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
				cpuReg.PC = gimeBus->readMemoryWord(VECTOR_IRQ);
			}
			else if (cpuSoftHalt == CPU_SOFTWARE_HALT_SYNC)
				cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
		}
		else if (assertedInterrupts[INT_RESET])
		{
			cpuReg.PC = gimeBus->readMemoryWord(VECTOR_RESET);
			assertedInterrupts[INT_RESET] &= ~INT_ASSERT_MASK_RESET;
			cpuCyclesTotal += 2;		// RESET interrupt uses 2 CPU cycles to complete
			cpuSoftHalt = CPU_SOFTWARE_HALT_NONE;
		}
		
		if (cpuSoftHalt != CPU_SOFTWARE_HALT_NONE)
			return;

		// CPU is not halted so here we go! First grab opcode
		debuggerRegPC = cpuReg.PC;				// For our Debugger, Preserve the start address of instruction before the PC gets incremented by the handlers


		curOpCode = readByteAtCurPC();			// Note: These functions auto-advance the PC by amount of bytes read

		if ((curOpCode == 0x10) || (curOpCode == 0x11))
		{
			// Since this is a double-byte operation, read the next byte and feed the combination to our pointer-grabbing function
			curOpCodeExtra = readByteAtCurPC();
			extendedOpCodePtrs((curOpCode * 256) + curOpCodeExtra);
			curOpCodeCycleCount = extendedOpCodeObjects.opBaseCycles;
			curOpMnemonic = extendedOpCodeObjects.mnemonicName;
			curAddrMode = (this->*extendedOpCodeObjects.addrModePtr)();
			if (curAddrMode == -1)		// Check if invalid instruction. If so, ignore first extended opcode prefix and treat 2nd byte as the actual opcode
			{
				curOpCode = curOpCodeExtra;
				curOpCodeCycleCount = mainOpCodeLookup[curOpCode].opBaseCycles;
				curOpMnemonic = mainOpCodeLookup[curOpCode].mnemonicName;
				curAddrMode = (this->*mainOpCodeLookup[curOpCode].addrModePtr)();
				if (curAddrMode == -1)		// Check one last time if we still have invalid opcode, if so abort/return
				{
					cpuSoftHalt = CPU_SOFTWARE_HALT_OTHER;
					printf("Halted CPU.\n");
					return;
				}
				(this->*mainOpCodeLookup[curOpCode].execOpPtr)(curAddrMode);
			}
			else
				(this->*extendedOpCodeObjects.execOpPtr)(curAddrMode);
		}
		else
		{
			curOpCodeCycleCount = mainOpCodeLookup[curOpCode].opBaseCycles;
			curOpMnemonic = mainOpCodeLookup[curOpCode].mnemonicName;
			// Next, we call the corresponding address mode function for the current instruction.
			// This will either populate our Effective address, or the operand byte(s) needed for the operation.
			curAddrMode = (this->*mainOpCodeLookup[curOpCode].addrModePtr)();
			if (curAddrMode == -1)		// Double-check for invalid instructions and abort/return if so
			{
				cpuSoftHalt = CPU_SOFTWARE_HALT_OTHER;
				printf("Halted CPU.\n");
				return;
			}
			// Now we execute our actual logic for the specified instruction
			(this->*mainOpCodeLookup[curOpCode].execOpPtr)(curAddrMode);
		}
		// Finally, set our counter to the total CPU cycles our current instruction just used so we can wait that many "ticks" before executing the
		// next instruction in order to syncronize with the other emulated hardware
		nextOpWaitCounter = curOpCodeCycleCount - 1;	// -1 since the actual all-at-once operation counts as the first "tick" in total CPU cycles needed for this operation	
		cpuCyclesTotal += curOpCodeCycleCount;
		instructionTotalCounter++;
		
		#ifdef DEBUGGER_ENABLED
			printDebugMsgs();
		#endif
	}
}

void Cpu6809::pushToStack(uint16_t* stackPtr)
{
	uint16_t stackAddr = *stackPtr;
	if (operandByte & MASK_PC)
	{
		stackAddr -= 2;
		curOpCodeCycleCount += 2;
		gimeBus->writeMemoryWord(stackAddr, cpuReg.PC);
	}
	if (operandByte & MASK_SU)
	{
		stackAddr -= 2;
		curOpCodeCycleCount += 2;
		if (stackPtr == &cpuReg.S)
			gimeBus->writeMemoryWord(stackAddr, cpuReg.U);
		else if (stackPtr == &cpuReg.U)
			gimeBus->writeMemoryWord(stackAddr, cpuReg.S);
	}
	if (operandByte & MASK_Y)
	{
		stackAddr -= 2;
		curOpCodeCycleCount += 2;
		gimeBus->writeMemoryWord(stackAddr, cpuReg.Y);
	}
	if (operandByte & MASK_X)
	{
		stackAddr -= 2;
		curOpCodeCycleCount += 2;
		gimeBus->writeMemoryWord(stackAddr, cpuReg.X);
	}
	if (operandByte & MASK_DP)
	{
		stackAddr--;
		curOpCodeCycleCount++;
		gimeBus->writeMemoryByte(stackAddr, cpuReg.DP);
	}
	if (operandByte & MASK_B)
	{
		stackAddr--;
		curOpCodeCycleCount++;
		gimeBus->writeMemoryByte(stackAddr, cpuReg.Acc.B);
	}
	if (operandByte & MASK_A)
	{
		stackAddr--;
		curOpCodeCycleCount++;
		gimeBus->writeMemoryByte(stackAddr, cpuReg.Acc.A);
	}
	if (operandByte & MASK_CC)
	{
		stackAddr--;
		curOpCodeCycleCount++;
		gimeBus->writeMemoryByte(stackAddr, cpuReg.CC.Byte);
	}
	*stackPtr = stackAddr;
}

void Cpu6809::pullFromStack(uint16_t* stackPtr)
{
	uint16_t stackAddr = *stackPtr;

	if (operandByte & MASK_CC)
	{
		curOpCodeCycleCount++;
		//byteToFlags(gimeBus->readMemoryByte(stackAddr));
		cpuReg.CC.Byte = gimeBus->readMemoryByte(stackAddr);
		stackAddr++;
	}
	if (operandByte & MASK_A)
	{
		curOpCodeCycleCount++;
		cpuReg.Acc.A = gimeBus->readMemoryByte(stackAddr);
		stackAddr++;
	}
	if (operandByte & MASK_B)
	{
		curOpCodeCycleCount++;
		cpuReg.Acc.B = gimeBus->readMemoryByte(stackAddr);
		stackAddr++;
	}
	if (operandByte & MASK_DP)
	{
		curOpCodeCycleCount++;
		cpuReg.DP = gimeBus->readMemoryByte(stackAddr);
		stackAddr++;
	}
	if (operandByte & MASK_X)
	{
		curOpCodeCycleCount += 2;
		cpuReg.X = gimeBus->readMemoryWord(stackAddr);
		stackAddr += 2;
	}
	if (operandByte & MASK_Y)
	{
		curOpCodeCycleCount += 2;
		cpuReg.Y = gimeBus->readMemoryWord(stackAddr);
		stackAddr += 2;
	}
	if (operandByte & MASK_SU)
	{
		curOpCodeCycleCount += 2;
		if (stackPtr == &cpuReg.S)
			cpuReg.U = gimeBus->readMemoryWord(stackAddr);
		else if (stackPtr == &cpuReg.U)
			cpuReg.S = gimeBus->readMemoryWord(stackAddr);
		stackAddr += 2;
	}
	if (operandByte & MASK_PC)
	{
		curOpCodeCycleCount += 2;
		cpuReg.PC = gimeBus->readMemoryWord(stackAddr);
		stackAddr += 2;
	}
	*stackPtr = stackAddr;
}

int Cpu6809::addrModeInherent()
{
	*curOpAddressString = NULL;
	return ADDR_INHERENT;
}

int Cpu6809::addrModeImmediateByte()
{
	operandByte = readByteAtCurPC();

	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "#$%02x", operandByte);
	return ADDR_IMMEDIATE_BYTE;
}

int Cpu6809::addrModeImmediateWord()
{
	operandWord = readWordAtCurPC();

	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "#$%04x", operandWord);
	return ADDR_IMMEDIATE_WORD;
}

int Cpu6809::addrModeImmediateRegs()
{
	operandByte = readByteAtCurPC();
	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "%s,%s", interRegNames[operandByte >> 4].c_str(), interRegNames[operandByte & 0x0F].c_str());
	return ADDR_IMMEDIATE_BYTE;
}

int Cpu6809::addrModeImmediateStack()
{
	operandByte = readByteAtCurPC();

	#ifdef DEBUGGER_ENABLED
	std::string disasmStackList;
	uint8_t maskByte = 0x01;

	if (operandByte & 0x01)
		disasmStackList += "CC";
	if (operandByte & 0x02)
		if (!disasmStackList.empty())
			disasmStackList += ",A";
		else
			disasmStackList += "A";
	if (operandByte & 0x04)
		if (!disasmStackList.empty())
			disasmStackList += ",B";
		else
			disasmStackList += "B"; 
	if (operandByte & 0x08)
		if (!disasmStackList.empty())
			disasmStackList += ",DP";
		else
			disasmStackList += "DP";
	if (operandByte & 0x10)
		if (!disasmStackList.empty())
			disasmStackList += ",X";
		else
			disasmStackList += "X";
	if (operandByte & 0x20)
		if (!disasmStackList.empty())
			disasmStackList += ",Y";
		else
			disasmStackList += "Y";
	if (operandByte & 0x40)
	{
		if (!disasmStackList.empty())
			disasmStackList += ",";
		if (curOpMnemonic.at(3) == 'S')
			disasmStackList += "U";
		else if (curOpMnemonic.at(3) == 'U')
			disasmStackList += "S";
	}
	if (operandByte & 0x80)
		if (!disasmStackList.empty())
			disasmStackList += ",PC";
		else
			disasmStackList += "PC";
	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "%s", disasmStackList.c_str());
	#endif

	return ADDR_IMMEDIATE_BYTE;
}

int Cpu6809::addrModeDirect()
{
	operandByte = readByteAtCurPC();
	effectiveAddr = ((cpuReg.DP * 256) + operandByte);

	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "<$%02x", operandByte);
	return ADDR_DIRECT;
}

int Cpu6809::addrModeExtended()
{
	operandWord = readWordAtCurPC();
	effectiveAddr = operandWord;

	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "$%04x", operandWord);
	return ADDR_EXTENDED;
}

int Cpu6809::addrModeIndexed()
{
	// This function will return the final 16-bit address in memory that the instruction ultimately accesses
	// depended on the specific Indexing mode. This is referred to as the "Effective Address".
	int8_t signedByte;
	int16_t signedWord;
	uint8_t paramByte = readByteAtCurPC();
	uint8_t registerID = (paramByte & 0b01100000) >> 5;
	char offsetSignChar = NULL;

	if (paramByte & 0x80)
	{
		switch (paramByte & 0b10011111)											// Strip off the Register ID (Bits 6-5) before comparing postbyte opcode
		{
			// "Constant offset from R" modes
		case 0b10000100:	// Non-Indirect addressing (No offset)
			effectiveAddr = *indexedRegOrder[registerID];						// Use the Register ID as an index to lookup pointer to actual register value in our code
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), ",%c", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10010100:	// Indirect addressing (No offset)
			curOpCodeCycleCount += 3;											// Uses an additional 3 cycles to perform the instruction
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID]);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[,%c]", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10001000:	// Non-Indirect addressing (8-bit offset)
			curOpCodeCycleCount++;												// Uses 1 additional cpu cycle
			signedByte = readByteAtCurPC();
			effectiveAddr = *indexedRegOrder[registerID] + signedByte;			// Cast the unsigned byte into a signed one
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "%d,%c", signedByte, indexRegName[registerID]);
			return ADDR_INDEXED_WORD;
		case 0b10011000:	// Indirect addressing (8-bit offset)
			curOpCodeCycleCount += 4;											// Uses 4 additional cycles
			signedByte = readByteAtCurPC();
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID] + signedByte);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[%d,%c]", signedByte, indexRegName[registerID]);
			return ADDR_INDEXED_WORD;
		case 0b10001001:	// Non-Indirect addressing (16-bit offset)
			curOpCodeCycleCount += 4;
			signedWord = readWordAtCurPC();
			effectiveAddr = *indexedRegOrder[registerID] + signedWord;		// Cast the unsigned word into a signed one
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "%d,%c", signedWord, indexRegName[registerID]);
			return ADDR_INDEXED_DWORD;
		case 0b10011001:	// Indirect addressing (16-bit offset)
			curOpCodeCycleCount += 7;
			signedWord = readWordAtCurPC();
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID] + signedWord);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[%d,%c]", signedWord, indexRegName[registerID]);
			return ADDR_INDEXED_DWORD;
			// "Accumulator offset from R" modes (signed)
		case 0b10000110:	// Non-Indirect addressing (A register offset)
			curOpCodeCycleCount++;												// Uses 1 additional cpu cycle
			effectiveAddr = *indexedRegOrder[registerID] + (int8_t)cpuReg.Acc.A;
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "A,%c", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10010110:	// Indirect addressing (A register offset)
			curOpCodeCycleCount += 4;											// Uses 4 additional cpu cycles
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID] + (int8_t)cpuReg.Acc.A);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[A,%c]", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10000101:	// Non-Indirect addressing (B register offset)
			curOpCodeCycleCount++;												// Uses 1 additional cpu cycle
			effectiveAddr = *indexedRegOrder[registerID] + (int8_t)cpuReg.Acc.B;
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "B,%c", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10010101:	// Indirect addressing (B register offset)
			curOpCodeCycleCount += 4;											// Uses 4 additional cpu cycles
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID] + (int8_t)cpuReg.Acc.B);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[B,%c]", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10001011:	// Non-Indirect addressing (D register offset)
			curOpCodeCycleCount += 4;
			effectiveAddr = *indexedRegOrder[registerID] + (int16_t)cpuReg.Acc.D;
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "D,%c", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10011011:	// Indirect addressing (D register offset)
			curOpCodeCycleCount += 7;
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID] + (int16_t)cpuReg.Acc.D);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[D,%c]", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
			// "Auto Increment/Decrement of R" modes
		case 0b10000000:	// Non-Indirect addressing (Post-increment by 1)
			curOpCodeCycleCount += 2;
			effectiveAddr = *indexedRegOrder[registerID];
			if ((curOpCode < 0x30) || (curOpCode > 0x33) || (opcodeToIndexRegLookup[curOpCode & 0x03]) != registerID)	// Skip auto-increment if LEAx instruction refers to itself in operand
				*indexedRegOrder[registerID] += 1;
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), ",%c+", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10000001:	// Non-Indirect addressing (Post-increment by 2)
			curOpCodeCycleCount += 3;
			effectiveAddr = *indexedRegOrder[registerID];
			if ((curOpCode < 0x30) || (curOpCode > 0x33) || (opcodeToIndexRegLookup[curOpCode & 0x03]) != registerID)	// Skip auto-increment if LEAx instruction refers to itself in operand
				*indexedRegOrder[registerID] += 2;
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), ",%c++", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10010001:	// Indirect addressing (Post-increment by 2)
			curOpCodeCycleCount += 6;
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID]);
			*indexedRegOrder[registerID] += 2;
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[,%c++]", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10000010:	// Non-Indirect addressing (Pre-decrement by 1)
			curOpCodeCycleCount += 2;
			*indexedRegOrder[registerID] -= 1;
			effectiveAddr = *indexedRegOrder[registerID];
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), ",-%c", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10000011:	// Non-Indirect addressing (Pre-decrement by 2)
			curOpCodeCycleCount += 3;
			*indexedRegOrder[registerID] -= 2;
			effectiveAddr = *indexedRegOrder[registerID];
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), ",--%c", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
		case 0b10010011:	// Indirect addressing (Pre-decrement by 2)
			curOpCodeCycleCount += 6;
			*indexedRegOrder[registerID] -= 2;
			effectiveAddr = gimeBus->readMemoryWord(*indexedRegOrder[registerID]);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[,--%c]", indexRegName[registerID]);
			return ADDR_INDEXED_BYTE;
			// "Constant Offset from PC" modes (signed)
		case 0b10001100:	// Non-Indirect addressing (8-bit offset)
			curOpCodeCycleCount++;
			signedByte = readByteAtCurPC();
			effectiveAddr = (cpuReg.PC + signedByte);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "%d,PCR", signedByte);
			return ADDR_INDEXED_WORD;
		case 0b10011100:	// Indirect addressing (8-bit offset)
			curOpCodeCycleCount += 4;
			signedByte = readByteAtCurPC();
			effectiveAddr = gimeBus->readMemoryWord(cpuReg.PC + signedByte);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[%d,PCR]", signedByte);
			return ADDR_INDEXED_WORD;
		case 0b10001101:	// Non-Indirect addressing (16-bit offset)
			curOpCodeCycleCount += 5;
			signedWord = readWordAtCurPC();
			effectiveAddr = (cpuReg.PC + signedWord);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "$%04x,PCR", signedWord);
			return ADDR_INDEXED_DWORD;
		case 0b10011101:	// Indirect addressing (16-bit offset)
			curOpCodeCycleCount += 8;
			signedWord = readWordAtCurPC();
			effectiveAddr = gimeBus->readMemoryWord(cpuReg.PC + signedWord);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[$%04x,PCR]", signedWord);
			return ADDR_INDEXED_DWORD;
			// "Extended Indirect" mode
		case 0b10011111:	// Indirect addressing (16-bit address)
			curOpCodeCycleCount += 5;
			operandWord = readWordAtCurPC();
			effectiveAddr = gimeBus->readMemoryWord(operandWord);
			sprintf_s(curOpAddressString, sizeof(curOpAddressString), "[$%04x]", operandWord);
			return ADDR_INDEXED_DWORD;
		}
	}
	else
	{
		// If here, the postbyte was less than 0x80 meaning this indexed mode is "Non-indirect Constant Offset from R (5-bit)".
		// This special-case mode has the 5-bit offset embedded in the postbyte opcode itself.
		signedByte = paramByte & 0b00011111;	// Bits 4-0 of the postbyte opcode contain our 5-bit offset. Strip off the rest
		if (signedByte > 15)
			signedByte -= 32;					// Converts unsigned to signed

		curOpCodeCycleCount++;													// Uses 1 additional cpu cycle
		effectiveAddr = *indexedRegOrder[registerID] + signedByte;
		sprintf_s(curOpAddressString, sizeof(curOpAddressString), "%d,%c", signedByte, indexRegName[registerID]);
	}
	return ADDR_INDEXED_BYTE;
}

int Cpu6809::addrModeRelativeByte()
{
	operandByte = readByteAtCurPC();
	effectiveAddr = cpuReg.PC + (int8_t)operandByte;

	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "$%04x", effectiveAddr);
	return ADDR_RELATIVE_BYTE;
}

int Cpu6809::addrModeRelativeWord()
{
	operandWord = readWordAtCurPC();
	effectiveAddr = cpuReg.PC + (int16_t)operandWord;

	sprintf_s(curOpAddressString, sizeof(curOpAddressString), "$%04x", effectiveAddr);
	return ADDR_RELATIVE_WORD;
}

int Cpu6809::invalidOpCode()
{
	// This functions handles invalid OpCodes 
	printf("Error: Invalid Op code = $%02X encountered @ $%04X.\n", curOpCode, debuggerRegPC);
	return (-1);
}

uint8_t Cpu6809::getParamByte(int addrMode)
{
	if (addrMode == ADDR_IMMEDIATE_BYTE)
		return operandByte;
	else
		return gimeBus->readMemoryByte(effectiveAddr);
}

uint16_t Cpu6809::getParamWord(int addrMode)
{
	if (addrMode == ADDR_IMMEDIATE_WORD)
		return operandWord;
	else
		return gimeBus->readMemoryWord(effectiveAddr);
}

void Cpu6809::ABX(int addrMode)
{
	cpuReg.X = cpuReg.X + cpuReg.Acc.B;
}

void Cpu6809::ADCA(int addrMode)
{
	paramByte = getParamByte(addrMode);	// This function will either return either "operandByte" for Immediate Address modes or the byte in Logical RAM pointed to by "effectiveAddr"
	// Perform a 4-bit version of operation to properly set the half-carry flag
	resultByte = (cpuReg.Acc.A & 0x0F) + (paramByte & 0x0F) + cpuReg.CC.C;
	cpuReg.CC.H = (resultByte > 0x0F);

	resultByte = cpuReg.Acc.A + paramByte + cpuReg.CC.C;
	resultWord = cpuReg.Acc.A + paramByte + cpuReg.CC.C;
	cpuReg.CC.V = (~(cpuReg.Acc.A ^ paramByte) & (cpuReg.Acc.A ^ resultByte)) & 0x80;
	cpuReg.CC.C = (resultWord > 0xFF);
	cpuReg.Acc.A = resultByte;
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
}

void Cpu6809::ADCB(int addrMode)
{
	paramByte = getParamByte(addrMode);	// This function will either return either "operandByte" for Immediate Address modes or the byte in Logical RAM pointed to by "effectiveAddr"
	// Perform a 4-bit version of operation to properly set the half-carry flag
	resultByte = (cpuReg.Acc.B & 0x0F) + (paramByte & 0x0F) + cpuReg.CC.C;
	cpuReg.CC.H = (resultByte > 0x0F);

	resultByte = cpuReg.Acc.B + paramByte + cpuReg.CC.C;
	resultWord = cpuReg.Acc.B + paramByte + cpuReg.CC.C;
	cpuReg.CC.V = (~(cpuReg.Acc.B ^ paramByte) & (cpuReg.Acc.B ^ resultByte)) & 0x80;
	cpuReg.CC.C = (resultWord > 0xFF);
	cpuReg.Acc.B = resultByte;
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
}

void Cpu6809::ADDA(int addrMode)
{
	paramByte = getParamByte(addrMode);		// This function will either return either "operandByte" for Immediate Address modes or the byte in Logical RAM pointed to by "effectiveAddr"
	// Perform a 4-bit version of operation to properly set the half-carry flag
	resultByte = (cpuReg.Acc.A & 0x0F) + (paramByte & 0x0F);
	cpuReg.CC.H = (resultByte > 0x0F);

	resultByte = cpuReg.Acc.A + paramByte;
	resultWord = cpuReg.Acc.A + paramByte;
	cpuReg.CC.V = (~(cpuReg.Acc.A ^ paramByte) & (cpuReg.Acc.A ^ resultByte)) & 0x80;
	cpuReg.CC.C = (resultWord > 0xFF);
	cpuReg.Acc.A = resultByte;
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
}

void Cpu6809::ADDB(int addrMode)
{
	paramByte = getParamByte(addrMode);		// This function will either return either "operandByte" for Immediate Address modes or the byte in Logical RAM pointed to by "effectiveAddr"
	// Perform a 4-bit version of operation to properly set the half-carry flag
	resultByte = (cpuReg.Acc.B & 0x0F) + (paramByte & 0x0F);
	cpuReg.CC.H = (resultByte > 0x0F);

	resultByte = cpuReg.Acc.B + paramByte;
	resultWord = cpuReg.Acc.B + paramByte;
	cpuReg.CC.V = (~(cpuReg.Acc.B ^ paramByte) & (cpuReg.Acc.B ^ resultByte)) & 0x80;
	cpuReg.CC.C = (resultWord > 0xFF);
	cpuReg.Acc.B = resultByte;
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
}

void Cpu6809::ADDD(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.Acc.D + paramWord;
	resultDWord = cpuReg.Acc.D + paramWord;
	cpuReg.CC.V = (~(cpuReg.Acc.D ^ paramWord) & (cpuReg.Acc.D ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (resultDWord > 0xFFFF);
	cpuReg.Acc.D = resultWord;
	cpuReg.CC.Z = !cpuReg.Acc.D;
	cpuReg.CC.N = (cpuReg.Acc.D & 0x8000);
}

void Cpu6809::ANDA(int addrMode)
{
	cpuReg.Acc.A = cpuReg.Acc.A & getParamByte(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.V = false;
}

void Cpu6809::ANDB(int addrMode)
{
	cpuReg.Acc.B = cpuReg.Acc.B & getParamByte(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.V = false;
}

void Cpu6809::ANDCC(int addrMode)
{
	cpuReg.CC.Byte &= operandByte;
}

void Cpu6809::ASL(int addrMode)
{
	resultByte = gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.C = (resultByte & 0x80);		// Carry flag will become the value of the current Negative sign bit 7 AFTER we do shift operation
	cpuReg.CC.V = (bool)(resultByte & 0x40) ^ cpuReg.CC.C;
	resultByte <<= 1;							// Shift "resultByte" to the left once
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::ASRA(int addrMode)
{
	resultByte = cpuReg.Acc.A;
	cpuReg.CC.C = (resultByte & 0x01);	
	resultByte = (int8_t)resultByte >> 1;		// "resultByte" is casted as signed becase on right arithmetic shifts, the sign bit 7 is preserved 
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::ASRB(int addrMode)
{
	resultByte = cpuReg.Acc.B;
	cpuReg.CC.C = (resultByte & 0x01);
	resultByte = (int8_t)resultByte >> 1;		// "resultByte" is casted as signed becase on right arithmetic shifts, the sign bit 7 is preserved 
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::ASR(int addrMode)
{
	resultByte = gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.C = (resultByte & 0x01);
	resultByte = (int8_t)resultByte >> 1;		// "resultByte" is casted as signed becase on right arithmetic shifts, the sign bit 7 is preserved 
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::BITA(int addrMode)
{
	resultByte = cpuReg.Acc.A & getParamByte(addrMode);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
}

void Cpu6809::BITB(int addrMode)
{
	resultByte = cpuReg.Acc.B & getParamByte(addrMode);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
}

void Cpu6809::CLRA(int addrMode)
{
	cpuReg.Acc.A = 0;
	cpuReg.CC.N = false;
	cpuReg.CC.Z = true;
	cpuReg.CC.V = false;
	cpuReg.CC.C = false;
}

void Cpu6809::CLRB(int addrMode)
{
	cpuReg.Acc.B = 0;
	cpuReg.CC.N = false;
	cpuReg.CC.Z = true;
	cpuReg.CC.V = false;
	cpuReg.CC.C = false;
}

void Cpu6809::CLR(int addrMode)
{
	// CLR instruction internally performs a READ operation on the bus, the result of which is discarded. But it's important to replicate that behavoir
	// here since it can do things like acknowledge interrupts, etc.
	gimeBus->readMemoryByte(effectiveAddr);		// Dummy read memory bus operation

	gimeBus->writeMemoryByte(effectiveAddr, 0);
	cpuReg.CC.N = false;
	cpuReg.CC.Z = true;
	cpuReg.CC.V = false;
	cpuReg.CC.C = false;
}

void Cpu6809::CMPA(int addrMode)
{
	paramByte = getParamByte(addrMode);		// This function will either return either "operandByte" for Immediate Address modes or the byte in Logical RAM pointed to by "effectiveAddr"
	// Now we simulate the subtraction, but we do it using a 16-bit variable which can represent the result if it exceeds the 8-bit range
	resultByte = cpuReg.Acc.A - paramByte;
	cpuReg.CC.V = ((cpuReg.Acc.A ^ paramByte) & (cpuReg.Acc.A ^ resultByte)) & 0x80;
	cpuReg.CC.C = (paramByte > cpuReg.Acc.A);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
}

void Cpu6809::CMPB(int addrMode)
{
	paramByte = getParamByte(addrMode);		// This function will either return either "operandByte" for Immediate Address modes or the byte in Logical RAM pointed to by "effectiveAddr"
	// Now we simulate the subtraction, but we do it using a 16-bit variable which can represent the result if it exceeds the 8-bit range
	resultByte = cpuReg.Acc.B - paramByte;
	cpuReg.CC.V = ((cpuReg.Acc.B ^ paramByte) & (cpuReg.Acc.B ^ resultByte)) & 0x80;
	cpuReg.CC.C = (paramByte > cpuReg.Acc.B);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
}

void Cpu6809::CMPD(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.Acc.D - paramWord;
	cpuReg.CC.V = ((cpuReg.Acc.D ^ paramWord) & (cpuReg.Acc.D ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (paramWord > cpuReg.Acc.D);
	cpuReg.CC.N = (resultWord & 0x8000);
	cpuReg.CC.Z = !resultWord;
}

void Cpu6809::CMPS(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.S - paramWord;
	cpuReg.CC.V = ((cpuReg.S ^ paramWord) & (cpuReg.S ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (paramWord > cpuReg.S);
	cpuReg.CC.N = (resultWord & 0x8000);
	cpuReg.CC.Z = !resultWord;
}

void Cpu6809::CMPU(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.U - paramWord;
	cpuReg.CC.V = ((cpuReg.U ^ paramWord) & (cpuReg.U ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (paramWord > cpuReg.U);
	cpuReg.CC.N = (resultWord & 0x8000);
	cpuReg.CC.Z = !resultWord;
}

void Cpu6809::CMPX(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.X - paramWord;
	cpuReg.CC.V = ((cpuReg.X ^ paramWord) & (cpuReg.X ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (paramWord > cpuReg.X);
	cpuReg.CC.N = (resultWord & 0x8000);
	cpuReg.CC.Z = !resultWord;
}

void Cpu6809::CMPY(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.Y - paramWord;
	cpuReg.CC.V = ((cpuReg.Y ^ paramWord) & (cpuReg.Y ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (paramWord > cpuReg.Y);
	cpuReg.CC.N = (resultWord & 0x8000);
	cpuReg.CC.Z = !resultWord;
}

void Cpu6809::COMA(int addrMode)
{
	resultByte = ~cpuReg.Acc.A;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
	cpuReg.CC.C = true;
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::COMB(int addrMode)
{
	resultByte = ~cpuReg.Acc.B;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
	cpuReg.CC.C = true;
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::COM(int addrMode)
{
	resultByte = ~gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
	cpuReg.CC.C = true;
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::CWAI(int addrMode)
{
	cpuReg.CC.Byte = cpuReg.CC.Byte & operandByte;
	cpuReg.CC.E = true;
	operandByte = MASK_CC | MASK_A | MASK_B | MASK_DP | MASK_X | MASK_Y | MASK_SU | MASK_PC;
	pushToStack(&cpuReg.S);			// Push ENTIRE machine state onto hardware stack
	cpuSoftHalt = CPU_SOFTWARE_HALT_CWAI;
}

void Cpu6809::DAA(int addrMode)
{
	// I'm grateful for this instruction because it lets me use the term "Nibble" in my code :D
	uint8_t highNibble = cpuReg.Acc.A >> 4;
	uint8_t lowNibble = cpuReg.Acc.A & 0x0F;
	if (cpuReg.CC.C || (highNibble > 9) || ((highNibble > 8) && (lowNibble > 9)))
		highNibble += 6;
	if (cpuReg.CC.H || (lowNibble > 9))
		lowNibble += 6;
	resultWord = (highNibble * 16) + lowNibble;
	cpuReg.CC.C = (resultWord > 0xFF);
	cpuReg.Acc.A = (uint8_t)resultWord;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
}

void Cpu6809::DECA(int addrMode)
{
	resultByte = cpuReg.Acc.A;
	cpuReg.CC.V = (resultByte == 0x80);
	resultByte--;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::DECB(int addrMode)
{
	resultByte = cpuReg.Acc.B;
	cpuReg.CC.V = (resultByte == 0x80);
	resultByte--;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::DEC(int addrMode)
{
	resultByte = gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.V = (resultByte == 0x80);
	resultByte--;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::EORA(int addrMode)
{
	resultByte = cpuReg.Acc.A ^ getParamByte(addrMode);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::EORB(int addrMode)
{
	resultByte = cpuReg.Acc.B ^ getParamByte(addrMode);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.V = false;
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::EXG(int addrMode)
{
	uint8_t sourceReg = operandByte >> 4;
	uint8_t destReg = operandByte & 0x0F;

	// 8-bit to 8-bit exchange
	if ((sourceReg >= 0x08) && (destReg >= 0x08))
	{
		sourceReg &= 0x07;		// Converts actual register IDs to a zero-based index
		destReg &= 0x07;		// Converts actual register IDs to a zero-based index
		paramByte = *interRegByteOrder[sourceReg];
		*interRegByteOrder[sourceReg] = *interRegByteOrder[destReg];
		*interRegByteOrder[destReg] = paramByte;
	}
	// 16-bit to 16-bit exchange
	else if ((sourceReg < 8) && (destReg < 8))
	{
		paramWord = *interRegWordOrder[sourceReg];
		*interRegWordOrder[sourceReg] = *interRegWordOrder[destReg];
		*interRegWordOrder[destReg] = paramWord;
	}
	// 8-bit to 16-bit exchange
	else if ((sourceReg >= 8) && (destReg < 8))
	{
		sourceReg &= 0x07;		// Converts actual register IDs to a zero-based index
		// First, handle special case when instruction is EXG A, D which produces same result as EXG A,B
		if ((sourceReg == 0) && (destReg == 0))
		{
			paramByte = *interRegByteOrder[sourceReg];
			*interRegByteOrder[sourceReg] = *interRegByteOrder[destReg];
			*interRegByteOrder[destReg] = paramByte;
		}
		// Next handle special case for when the 8-bit register to swap is CC or DP
		else if ((sourceReg == 2) || (sourceReg == 3))
		{
			paramByte = *interRegByteOrder[sourceReg];
			*interRegByteOrder[sourceReg] = (*interRegWordOrder[destReg] & 0x00FF);
			*interRegWordOrder[destReg] = (paramByte * 256) + paramByte;
		}
		// Finally handle case where 8-bit register is A or B
		else if ((sourceReg == 0) || (sourceReg == 1))
		{
			paramByte = *interRegByteOrder[sourceReg];
			*interRegByteOrder[sourceReg] = (*interRegWordOrder[destReg] & 0x00FF);
			*interRegWordOrder[destReg] = 0xFF00 + paramByte;
		}
		// Handle invalid register references
		else
		{
			*interRegByteOrder[sourceReg] = (*interRegWordOrder[destReg] & 0x00FF);
			*interRegWordOrder[destReg] = 0xFFFF;
		}
	}
	// 16-bit to 8-bit exchange
	else if ((sourceReg < 8) && (destReg >= 8))
	{
		destReg &= 0x07;		// Converts actual register IDs to a zero-based index
		paramWord = *interRegWordOrder[sourceReg];
		*interRegWordOrder[sourceReg] = 0xFF00 + *interRegByteOrder[destReg];
		*interRegByteOrder[destReg] = (paramWord & 0x00FF);
	}
}

void Cpu6809::INCA(int addrMode)
{
	resultByte = cpuReg.Acc.A;
	cpuReg.CC.V = (resultByte == 0x7F);
	resultByte++;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::INCB(int addrMode)
{
	resultByte = cpuReg.Acc.B;
	cpuReg.CC.V = (resultByte == 0x7F);
	resultByte++;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::INC(int addrMode)
{
	resultByte = gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.V = (resultByte == 0x7F);
	resultByte++;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::JMP(int addrMode)
{
	// Unlike most other address modes, JMP directly uses the effective address (just like LEAx instructions do) unless it's an Indirect mode
	cpuReg.PC = effectiveAddr;		
}

void Cpu6809::JSR(int addrMode)
{
	// First push our return address (currently in PC) to the stack
	cpuReg.S -= 2;
	gimeBus->writeMemoryWord(cpuReg.S, cpuReg.PC);
	// Now do the unconditional jump
	cpuReg.PC = effectiveAddr;
}

void Cpu6809::LDA(int addrMode)
{
	cpuReg.Acc.A = getParamByte(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.V = false;
}

void Cpu6809::LDB(int addrMode)
{
	cpuReg.Acc.B = getParamByte(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.V = false;
}

void Cpu6809::LDD(int addrMode)
{
	cpuReg.Acc.D = getParamWord(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.D & 0x8000);
	cpuReg.CC.Z = !cpuReg.Acc.D;
	cpuReg.CC.V = false;
}

void Cpu6809::LDS(int addrMode)
{
	cpuReg.S = getParamWord(addrMode);
	cpuReg.CC.N = (cpuReg.S & 0x8000);
	cpuReg.CC.Z = !cpuReg.S;
	cpuReg.CC.V = false;
}

void Cpu6809::LDU(int addrMode)
{
	cpuReg.U = getParamWord(addrMode);
	cpuReg.CC.N = (cpuReg.U & 0x8000);
	cpuReg.CC.Z = !cpuReg.U;
	cpuReg.CC.V = false;
}

void Cpu6809::LDX(int addrMode)
{
	cpuReg.X = getParamWord(addrMode);
	cpuReg.CC.N = (cpuReg.X & 0x8000);
	cpuReg.CC.Z = !cpuReg.X;
	cpuReg.CC.V = false;
}

void Cpu6809::LDY(int addrMode)
{
	cpuReg.Y = getParamWord(addrMode);
	cpuReg.CC.N = (cpuReg.Y & 0x8000);
	cpuReg.CC.Z = !cpuReg.Y;
	cpuReg.CC.V = false;
}

void Cpu6809::LEAS(int addrMode)
{
	cpuReg.S = effectiveAddr;
}

void Cpu6809::LEAU(int addrMode)
{
	cpuReg.U = effectiveAddr;
}

void Cpu6809::LEAX(int addrMode)
{
	cpuReg.X = effectiveAddr;
	cpuReg.CC.Z = !cpuReg.X;
}

void Cpu6809::LEAY(int addrMode)
{
	cpuReg.Y = effectiveAddr;
	cpuReg.CC.Z = !cpuReg.Y;
}

void Cpu6809::LSLA(int addrMode)
{
	cpuReg.CC.C = (cpuReg.Acc.A & 0x80);		// Carry flag will become the value of the current Negative sign bit 7 AFTER we do shift operation
	cpuReg.CC.V = (bool)(cpuReg.Acc.A & 0x40) ^ cpuReg.CC.C;
	cpuReg.Acc.A <<= 1;						
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
}

void Cpu6809::LSLB(int addrMode)
{
	cpuReg.CC.C = (cpuReg.Acc.B & 0x80);		// Carry flag will become the value of the current Negative sign bit 7 AFTER we do shift operation
	cpuReg.CC.V = (bool)(cpuReg.Acc.B & 0x40) ^ cpuReg.CC.C;
	cpuReg.Acc.B <<= 1;
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
}

void Cpu6809::LSL(int addrMode)
{
	resultByte = getParamByte(addrMode);
	cpuReg.CC.C = (resultByte & 0x80);		// Carry flag will become the value of the current Negative sign bit 7 AFTER we do shift operation
	cpuReg.CC.V = (bool)(resultByte & 0x40) ^ cpuReg.CC.C;
	resultByte <<= 1;
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::LSRA(int addrMode)
{
	cpuReg.CC.C = (cpuReg.Acc.A & 0x01);
	cpuReg.Acc.A >>= 1;
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
}

void Cpu6809::LSRB(int addrMode)
{
	cpuReg.CC.C = (cpuReg.Acc.B & 0x01);
	cpuReg.Acc.B >>= 1;
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
}

void Cpu6809::LSR(int addrMode)
{
	resultByte = getParamByte(addrMode);
	cpuReg.CC.C = (resultByte & 0x01);
	resultByte >>= 1;
	cpuReg.CC.Z = !resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::MUL(int addrMode)
{
	cpuReg.Acc.D = cpuReg.Acc.A * cpuReg.Acc.B;
	cpuReg.CC.C = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.D;
}

void Cpu6809::NEGA(int addrMode)
{
	cpuReg.CC.C = cpuReg.Acc.A;				// Should be true if not-zero, false if zero
	cpuReg.CC.V = (cpuReg.Acc.A == 0x80);
	cpuReg.Acc.A = 0 - cpuReg.Acc.A;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
}

void Cpu6809::NEGB(int addrMode)
{
	cpuReg.CC.C = cpuReg.Acc.B;				// Should be true if not-zero, false if zero
	cpuReg.CC.V = (cpuReg.Acc.B == 0x80);
	cpuReg.Acc.B = 0 - cpuReg.Acc.B;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
}

void Cpu6809::NEG(int addrMode)
{
	resultByte = getParamByte(addrMode);
	cpuReg.CC.C = resultByte;
	cpuReg.CC.V = (resultByte == 0x80);
	resultByte = 0 - resultByte;
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::NOP(int addrMode)
{
	// Do no operation :)
}

void Cpu6809::ORA(int addrMode)
{
	cpuReg.Acc.A |= getParamByte(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.V = false;
}

void Cpu6809::ORB(int addrMode)
{
	cpuReg.Acc.B |= getParamByte(addrMode);
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.V = false;
}

void Cpu6809::ORCC(int addrMode)
{
	cpuReg.CC.Byte |= operandByte;
}

void Cpu6809::PSHS(int addrMode)
{
	pushToStack(&cpuReg.S);
}

void Cpu6809::PSHU(int addrMode)
{
	pushToStack(&cpuReg.U);
}

void Cpu6809::PULS(int addrMode)
{
	pullFromStack(&cpuReg.S);
}

void Cpu6809::PULU(int addrMode)
{
	pullFromStack(&cpuReg.U);
}

void Cpu6809::ROLA(int addrMode)
{
	cpuReg.CC.V = (bool)(cpuReg.Acc.A & 0x80) ^ (bool)(cpuReg.Acc.A & 0x40);
	resultByte = (cpuReg.Acc.A << 1) | (uint8_t)cpuReg.CC.C;
	cpuReg.CC.C = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::ROLB(int addrMode)
{
	cpuReg.CC.V = (bool)(cpuReg.Acc.B & 0x80) ^ (bool)(cpuReg.Acc.B & 0x40);
	resultByte = (cpuReg.Acc.B << 1) | (uint8_t)cpuReg.CC.C;
	cpuReg.CC.C = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::ROL(int addrMode)
{
	paramByte = gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.V = (bool)(paramByte & 0x80) ^ (bool)(paramByte & 0x40);
	resultByte = (paramByte << 1) | (uint8_t)cpuReg.CC.C;
	cpuReg.CC.C = (paramByte & 0x80);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::RORA(int addrMode)
{
	resultByte = (cpuReg.Acc.A >> 1) | (cpuReg.CC.C * 0x80);
	cpuReg.CC.C = (cpuReg.Acc.A & 0x01);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.A = resultByte;
}

void Cpu6809::RORB(int addrMode)
{
	resultByte = (cpuReg.Acc.B >> 1) | (cpuReg.CC.C * 0x80);
	cpuReg.CC.C = (cpuReg.Acc.B & 0x01);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	cpuReg.Acc.B = resultByte;
}

void Cpu6809::ROR(int addrMode)
{
	paramByte = gimeBus->readMemoryByte(effectiveAddr);
	resultByte = (paramByte >> 1) | (cpuReg.CC.C * 0x80);
	cpuReg.CC.C = (paramByte & 0x01);
	cpuReg.CC.N = (resultByte & 0x80);
	cpuReg.CC.Z = !resultByte;
	gimeBus->writeMemoryByte(effectiveAddr, resultByte);
}

void Cpu6809::RTI(int addrMode)
{
	operandByte = MASK_CC;
	pullFromStack(&cpuReg.S);
	if (cpuReg.CC.E)
		operandByte = MASK_A | MASK_B | MASK_DP | MASK_X | MASK_Y | MASK_SU | MASK_PC;
	else
		operandByte = MASK_PC;
	pullFromStack(&cpuReg.S);
}

void Cpu6809::RTS(int addrMode)
{
	operandByte = MASK_PC;
	pullFromStack(&cpuReg.S);
}

void Cpu6809::SBCA(int addrMode)
{
	paramWord = getParamByte(addrMode) + cpuReg.CC.C;
	resultWord = cpuReg.Acc.A - paramWord;
	cpuReg.CC.V = ((cpuReg.Acc.A ^ (uint8_t)paramWord) & (cpuReg.Acc.A ^ (uint8_t)resultWord)) & 0x80;
	cpuReg.CC.C = (paramWord > cpuReg.Acc.A);
	cpuReg.Acc.A = (uint8_t)resultWord;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
}

void Cpu6809::SBCB(int addrMode)
{
	paramWord = getParamByte(addrMode) + cpuReg.CC.C;
	resultWord = cpuReg.Acc.B - paramWord;
	cpuReg.CC.V = ((cpuReg.Acc.B ^ (uint8_t)paramWord) & (cpuReg.Acc.B ^ (uint8_t)resultWord)) & 0x80;
	cpuReg.CC.C = (paramWord > cpuReg.Acc.B);
	cpuReg.Acc.B = (uint8_t)resultWord;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
}

void Cpu6809::SEX(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.Acc.A = cpuReg.CC.N ? 0xFF : 0x00;
	cpuReg.CC.Z = !cpuReg.Acc.D;
}

void Cpu6809::STA(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryByte(effectiveAddr, cpuReg.Acc.A);
}

void Cpu6809::STB(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryByte(effectiveAddr, cpuReg.Acc.B);
}

void Cpu6809::STD(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Acc.D & 0x8000);
	cpuReg.CC.Z = !cpuReg.Acc.D;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryWord(effectiveAddr, cpuReg.Acc.D);
}

void Cpu6809::STS(int addrMode)
{
	cpuReg.CC.N = (cpuReg.S & 0x8000);
	cpuReg.CC.Z = !cpuReg.S;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryWord(effectiveAddr, cpuReg.S);
}

void Cpu6809::STU(int addrMode)
{
	cpuReg.CC.N = (cpuReg.U & 0x8000);
	cpuReg.CC.Z = !cpuReg.U;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryWord(effectiveAddr, cpuReg.U);
}

void Cpu6809::STX(int addrMode)
{
	cpuReg.CC.N = (cpuReg.X & 0x8000);
	cpuReg.CC.Z = !cpuReg.X;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryWord(effectiveAddr, cpuReg.X);
}

void Cpu6809::STY(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Y & 0x8000);
	cpuReg.CC.Z = !cpuReg.Y;
	cpuReg.CC.V = false;
	gimeBus->writeMemoryWord(effectiveAddr, cpuReg.Y);
}

void Cpu6809::SUBA(int addrMode)
{
	paramByte = getParamByte(addrMode);
	resultByte = cpuReg.Acc.A - paramByte;
	cpuReg.CC.V = ((cpuReg.Acc.A ^ paramByte) & (cpuReg.Acc.A ^ resultByte)) & 0x80;
	cpuReg.CC.C = (paramByte > cpuReg.Acc.A);
	cpuReg.Acc.A = resultByte;
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
}

void Cpu6809::SUBB(int addrMode)
{
	paramByte = getParamByte(addrMode);
	resultByte = cpuReg.Acc.B - paramByte;
	cpuReg.CC.V = ((cpuReg.Acc.B ^ paramByte) & (cpuReg.Acc.B ^ resultByte)) & 0x80;
	cpuReg.CC.C = (paramByte > cpuReg.Acc.B);
	cpuReg.Acc.B = resultByte;
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
}

void Cpu6809::SUBD(int addrMode)
{
	paramWord = getParamWord(addrMode);
	resultWord = cpuReg.Acc.D - paramWord;
	cpuReg.CC.V = ((cpuReg.Acc.D ^ paramWord) & (cpuReg.Acc.D ^ resultWord)) & 0x8000;
	cpuReg.CC.C = (paramWord > cpuReg.Acc.D);
	cpuReg.Acc.D = resultWord;
	cpuReg.CC.N = (cpuReg.Acc.D & 0x8000);
	cpuReg.CC.Z = !cpuReg.Acc.D;
}

void Cpu6809::SWI(int addrMode)
{
	cpuReg.CC.E = true;
	operandByte = 0xFF;		// 0xFF = every bit set such that all registers are pushed to hardware stack
	pushToStack(&cpuReg.S);
	// Unlike the other 2, this SWI sets both interrupt mask bits in the CC register
	cpuReg.CC.I = true;
	cpuReg.CC.F = true;
	cpuReg.PC = gimeBus->readMemoryWord(VECTOR_SWI);
}

void Cpu6809::SWI2(int addrMode)
{
	cpuReg.CC.E = true;
	operandByte = 0xFF;		// 0xFF = every bit set such that all registers are pushed to hardware stack
	pushToStack(&cpuReg.S);
	cpuReg.PC = gimeBus->readMemoryWord(VECTOR_SWI2);
}

void Cpu6809::SWI3(int addrMode)
{
	cpuReg.CC.E = true;
	operandByte = 0xFF;		// 0xFF = every bit set such that all registers are pushed to hardware stack
	pushToStack(&cpuReg.S);
	cpuReg.PC = gimeBus->readMemoryWord(VECTOR_SWI3);
}

void Cpu6809::SYNC(int addrMode)
{
	cpuSoftHalt = CPU_SOFTWARE_HALT_SYNC;
}

void Cpu6809::TFR(int addrMode)
{
	uint8_t sourceReg = operandByte >> 4;
	uint8_t destReg = operandByte & 0x0F;

	// 8-bit to 8-bit transfer
	if ((sourceReg >= 0x08) && (destReg >= 0x08))
	{
		sourceReg &= 0x07;		// Converts actual register IDs to a zero-based index
		destReg &= 0x07;		// Converts actual register IDs to a zero-based index
		*interRegByteOrder[destReg] = *interRegByteOrder[sourceReg];
	}
	// 16-bit to 16-bit transfer
	else if ((sourceReg < 8) && (destReg < 8))
		*interRegWordOrder[destReg] = *interRegWordOrder[sourceReg];
	// 8-bit to 16-bit transfer
	else if ((sourceReg >= 8) && (destReg < 8))
	{
		sourceReg &= 0x07;		// Converts actual register IDs to a zero-based index
		// First handle special case for when the 8-bit register to transfer is CC or DP
		if ((sourceReg == 2) || (sourceReg == 3))
			*interRegWordOrder[destReg] = (*interRegByteOrder[sourceReg] * 256) + *interRegByteOrder[sourceReg];
		// Now handle case where 8-bit register is A or B
		else if ((sourceReg == 0) || (sourceReg == 1))
			*interRegWordOrder[destReg] = 0xFF00 + *interRegByteOrder[sourceReg];
		// Handle invalid register references
		else
			*interRegWordOrder[destReg] = 0xFFFF;
	}
	// 16-bit to 8-bit transfer
	else if ((sourceReg < 8) && (destReg >= 8))
	{
		destReg &= 0x07;		// Converts actual register IDs to a zero-based index
		*interRegByteOrder[destReg] = (*interRegWordOrder[sourceReg] & 0x00FF);
	}
}

void Cpu6809::TSTA(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Acc.A & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.A;
	cpuReg.CC.V = false;
}

void Cpu6809::TSTB(int addrMode)
{
	cpuReg.CC.N = (cpuReg.Acc.B & 0x80);
	cpuReg.CC.Z = !cpuReg.Acc.B;
	cpuReg.CC.V = false;
}

void Cpu6809::TST(int addrMode)
{
	paramByte = gimeBus->readMemoryByte(effectiveAddr);
	cpuReg.CC.N = (paramByte & 0x80);
	cpuReg.CC.Z = !paramByte;
	cpuReg.CC.V = false;
}

void Cpu6809::BCC(int addrMode)
{
	if (!cpuReg.CC.C)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BEQ(int addrMode)
{
	if (cpuReg.CC.Z)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BGE(int addrMode)
{
	if (cpuReg.CC.N == cpuReg.CC.V)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BGT(int addrMode)
{
	if ((cpuReg.CC.N == cpuReg.CC.V) && (!cpuReg.CC.Z))
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BHI(int addrMode)
{
	if (!cpuReg.CC.Z && !cpuReg.CC.C)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BLE(int addrMode)
{
	if ((cpuReg.CC.N != cpuReg.CC.V) || (cpuReg.CC.Z))
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BCS(int addrMode)
{
	if (cpuReg.CC.C)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BLS(int addrMode)
{
	if (cpuReg.CC.Z || cpuReg.CC.C)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BLT(int addrMode)
{
	if (cpuReg.CC.N != cpuReg.CC.V)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BMI(int addrMode)
{
	if (cpuReg.CC.N)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BNE(int addrMode)
{
	if (!cpuReg.CC.Z)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BPL(int addrMode)
{
	if (!cpuReg.CC.N)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BRA(int addrMode)
{
	cpuReg.PC = effectiveAddr;
}

void Cpu6809::BRN(int addrMode)
{
	// Never branch :)
}

void Cpu6809::BSR(int addrMode)
{
	cpuReg.S -= 2;
	gimeBus->writeMemoryWord(cpuReg.S, cpuReg.PC);
	cpuReg.PC = effectiveAddr;
}

void Cpu6809::BVC(int addrMode)
{
	if (!cpuReg.CC.V)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::BVS(int addrMode)
{
	if (cpuReg.CC.V)
		cpuReg.PC = effectiveAddr;
}

void Cpu6809::LBEQ(int addrMode)
{
	if (cpuReg.CC.Z)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBGE(int addrMode)
{
	if (cpuReg.CC.N == cpuReg.CC.V)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBGT(int addrMode)
{
	if ((cpuReg.CC.N == cpuReg.CC.V) && (!cpuReg.CC.Z))
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBHI(int addrMode)
{
	if (!cpuReg.CC.Z && !cpuReg.CC.C)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBCC(int addrMode)
{
	if (!cpuReg.CC.C)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBLE(int addrMode)
{
	if ((cpuReg.CC.N != cpuReg.CC.V) || cpuReg.CC.Z)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBCS(int addrMode)
{
	if (cpuReg.CC.C)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBLS(int addrMode)
{
	if (cpuReg.CC.Z || cpuReg.CC.C)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBLT(int addrMode)
{
	if (cpuReg.CC.N != cpuReg.CC.V)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBMI(int addrMode)
{
	if (cpuReg.CC.N)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBNE(int addrMode)
{
	if (!cpuReg.CC.Z)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBPL(int addrMode)
{
	if (!cpuReg.CC.N)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBRA(int addrMode)
{
	cpuReg.PC = effectiveAddr;
}

void Cpu6809::LBRN(int addrMode)
{
	// Never branch, long or otherwise :)
}

void Cpu6809::LBSR(int addrMode)
{
	cpuReg.S -= 2;
	gimeBus->writeMemoryWord(cpuReg.S, cpuReg.PC);
	cpuReg.PC = effectiveAddr;
}

void Cpu6809::LBVC(int addrMode)
{
	if (!cpuReg.CC.V)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::LBVS(int addrMode)
{
	if (cpuReg.CC.V)
	{
		cpuReg.PC = effectiveAddr;
		curOpCodeCycleCount++;			// Conditiional long branches require an extra cpu cycle when the branch is taken
	}
}

void Cpu6809::extendedOpCodePtrs(uint16_t opCodeWord)
{
	switch (opCodeWord)
	{
	// First we check for the Page 2 extended instructions
	case 0x1021:
		extendedOpCodeObjects = { "LBRN ", 5, RL2, &c::LBRN };
		break;
	case 0x1022:
		extendedOpCodeObjects = { "LBHI ", 5, RL2, &c::LBHI };
		break;
	case 0x1023:
		extendedOpCodeObjects = { "LBLS ",5,RL2,&c::LBLS };
		break;
	case 0x1024:
		extendedOpCodeObjects = { "LBCC ",5,RL2,&c::LBCC };
		break;
	case 0x1025:
		extendedOpCodeObjects = { "LBCS ",5,RL2,&c::LBCS };
		break;
	case 0x1026:
		extendedOpCodeObjects = { "LBNE ", 5, RL2, &c::LBNE };
		break;
	case 0x1027:
		extendedOpCodeObjects = { "LBEQ ",5,RL2,&c::LBEQ };
		break;
	case 0x1028:
		extendedOpCodeObjects = { "LBVC ", 5, RL2, &c::LBVC };
		break;
	case 0x1029:
		extendedOpCodeObjects = { "LBVS ", 5, RL2, &c::LBVS };
		break;
	case 0x102A:
		extendedOpCodeObjects = { "LBPL ", 5, RL2, &c::LBPL };
		break;
	case 0x102B:
		extendedOpCodeObjects = { "LBMI ", 5, RL2, &c::LBMI };
		break;
	case 0x102C:
		extendedOpCodeObjects = { "LBGE ", 5, RL2, &c::LBGE };
		break;
	case 0x102D:
		extendedOpCodeObjects = { "LBLT ", 5, RL2, &c::LBLT };
		break;
	case 0x102E:
		extendedOpCodeObjects = { "LBGT ", 5, RL2, &c::LBGT };
		break;
	case 0x102F:
		extendedOpCodeObjects = { "LBLE ", 5, RL2, &c::LBLE };
		break;
	case 0x103F:
		extendedOpCodeObjects = { "SWI2 ", 20, INH, &c::SWI2 };
		break;
	case 0x1083:
		extendedOpCodeObjects = { "CMPD ", 5, IM2, &c::CMPD };
		break;
	case 0x108C:
		extendedOpCodeObjects = { "CMPY ", 5, IM2, &c::CMPY };
		break;
	case 0x108E:
		extendedOpCodeObjects = { "LDY  ", 5, IM2, &c::LDY };
		break;
	case 0x1093:
		extendedOpCodeObjects = { "CMPD ", 7, DIR, &c::CMPD };
		break;
	case 0x109C:
		extendedOpCodeObjects = { "CMPY ", 7, DIR, &c::CMPY };
		break;
	case 0x109E:
		extendedOpCodeObjects = { "LDY  ", 6, DIR, &c::LDY };
		break;
	case 0x109F:
		extendedOpCodeObjects = { "STY  ", 6, DIR, &c::STY };
		break;
	case 0x10A3:
		extendedOpCodeObjects = { "CMPD ", 7, IDX, &c::CMPD };
		break;
	case 0x10AC:
		extendedOpCodeObjects = { "CMPY ", 7, IDX, &c::CMPY };
		break;
	case 0x10AE:
		extendedOpCodeObjects = { "LDY  ", 6, IDX, &c::LDY };
		break;
	case 0x10AF:
		extendedOpCodeObjects = { "STY  ", 6, IDX, &c::STY };
		break;
	case 0x10B3:
		extendedOpCodeObjects = { "CMPD ", 8, EXT, &c::CMPD };
		break;
	case 0x10BC:
		extendedOpCodeObjects = { "CMPY ", 8, EXT, &c::CMPY };
		break;
	case 0x10BE:
		extendedOpCodeObjects = { "LDY  ", 7, EXT, &c::LDY };
		break;
	case 0x10BF:
		extendedOpCodeObjects = { "STY  ", 7, EXT, &c::STY };
		break;
	case 0x10CE:
		extendedOpCodeObjects = { "LDS  ", 4, IM2, &c::LDS };
		break;
	case 0x10DE:
		extendedOpCodeObjects = { "LDS  ", 6, DIR, &c::LDS };
		break;
	case 0x10DF:
		extendedOpCodeObjects = { "STS  ", 6, DIR, &c::STS };
		break;
	case 0x10EE:
		extendedOpCodeObjects = { "LDS  ", 6, IDX, &c::LDS };
		break;
	case 0x10EF:
		extendedOpCodeObjects = { "STS  ", 6, IDX, &c::STS };
		break;
	case 0x10FE:
		extendedOpCodeObjects = { "LDS  ", 7, EXT, &c::LDS };
		break;
	case 0x10FF:
		extendedOpCodeObjects = { "STS  ", 7, EXT, &c::STS };
		break;
	// And now we check the Page 3 extended instructions
	case 0x113F:
		extendedOpCodeObjects = { "SWI3 ",20,INH,&c::SWI3 };
		break;
	case 0x1183:
		extendedOpCodeObjects = { "CMPU ",5,IM2,&c::CMPU };
		break;
	case 0x118C:
		extendedOpCodeObjects = { "CMPS ", 5, IM2,&c::CMPS };
		break;
	case 0x1193:
		extendedOpCodeObjects = { "CMPU ", 7, DIR,&c::CMPU };
		break;
	case 0x119C:
		extendedOpCodeObjects = { "CMPS ", 7, DIR,&c::CMPS };
		break;
	case 0x11A3:
		extendedOpCodeObjects = { "CMPU ", 7, IDX, &c::CMPU };
		break;
	case 0x11AC:
		extendedOpCodeObjects = { "CMPS ",7,IDX,&c::CMPS };
		break;
	case 0x11B3:
		extendedOpCodeObjects = { "CMPU ", 8, EXT, &c::CMPU };
		break;
	case 0x11BC:
		extendedOpCodeObjects = { "CMPS ", 8, EXT, &c::CMPS };
		break;
	default:
		extendedOpCodeObjects = { "???  ", 0, ERR, NUL };
		break;
	}
}

void Cpu6809::printDebugMsgs()
{
	std::string outputBuffer;
	char disasmString[40] = { 0 }, flagsString[70] = { 0 };

	if (debuggerRegPC == breakpointAddress)
		debuggerStepEnabled = true;

	if (debuggerStepEnabled)
	{
		sprintf_s(flagsString, sizeof(flagsString), "cc=%02x a=%02x b=%02x e=00 f=00 dp=%02x x=%04x y=%04x u=%04x s=%04x v=0000\r\n", cpuReg.CC.Byte, cpuReg.Acc.A, cpuReg.Acc.B, cpuReg.DP, cpuReg.X, cpuReg.Y, cpuReg.U, cpuReg.S);
		switch (curAddrMode)
		{
		case ADDR_INHERENT:
			if ((curOpCode == 0x10) || (curOpCode == 0x11))
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %04x      ", debuggerRegPC, gimeBus->readMemoryWord(debuggerRegPC));
			else
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %02x          ", debuggerRegPC, curOpCode);
			break;
		case ADDR_IMMEDIATE_BYTE:
		case ADDR_INDEXED_BYTE:
		case ADDR_DIRECT:
		case ADDR_RELATIVE_BYTE:
			if ((curOpCode == 0x10) || (curOpCode == 0x11))
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %02x%04x      ", debuggerRegPC, curOpCode, gimeBus->readMemoryWord(debuggerRegPC + 1));
			else
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %04x        ", debuggerRegPC, gimeBus->readMemoryWord(debuggerRegPC));
			break;
		case ADDR_IMMEDIATE_WORD:
		case ADDR_INDEXED_WORD:
		case ADDR_RELATIVE_WORD:
		case ADDR_EXTENDED:
			if ((curOpCode == 0x10) || (curOpCode == 0x11))
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %04x%04x    ", debuggerRegPC, gimeBus->readMemoryWord(debuggerRegPC), gimeBus->readMemoryWord(debuggerRegPC + 2));
			else
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %02x%04x      ", debuggerRegPC, gimeBus->readMemoryByte(debuggerRegPC), gimeBus->readMemoryWord(debuggerRegPC + 1));
			break;
		case ADDR_INDEXED_DWORD:
			if ((curOpCode == 0x10) || (curOpCode == 0x11))
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %04x%02x%04x    ", debuggerRegPC, gimeBus->readMemoryWord(debuggerRegPC), gimeBus->readMemoryByte(debuggerRegPC + 2), gimeBus->readMemoryWord(debuggerRegPC + 3));
			else
				sprintf_s(disasmString, sizeof(disasmString), "%04x| %04x%04x    ", debuggerRegPC, gimeBus->readMemoryWord(debuggerRegPC), gimeBus->readMemoryWord(debuggerRegPC + 2));
			break;
		}

		outputBuffer = disasmString + curOpMnemonic + "   " + curOpAddressString;
		for (int i = outputBuffer.length(); i < 46; i++)
			outputBuffer += ' ';
		outputBuffer += flagsString;

		printf(outputBuffer.c_str());
		char inKey = getchar();
		if (inKey == 'q')
			debuggerStepEnabled = false;

		//fwrite(outputBuffer.c_str(), 1, outputBuffer.length(), traceListFile);
	}
}

void Cpu6809::printDisassembly(uint16_t logicalStartAddr, uint8_t* instructionPtr)
{
	uint8_t opCode = *instructionPtr;
	uint8_t addrMode = (this->*mainOpCodeLookup[opCode].addrModePtr)();
	
	switch (addrMode)
	{
	case ADDR_INHERENT:
		if ((opCode == 0x10) || (opCode == 0x11))
			printf("%04X %02X%02X      ", logicalStartAddr, opCode, *(instructionPtr + 1));
		else
			printf("%04X %02X          ", logicalStartAddr, opCode);
		break;
	case ADDR_IMMEDIATE_BYTE:
	case ADDR_INDEXED_BYTE:
	case ADDR_DIRECT:
	case ADDR_RELATIVE_BYTE:
		if ((curOpCode == 0x10) || (curOpCode == 0x11))
			printf("%04X %02X%02X%02X      ", logicalStartAddr, opCode, *(instructionPtr + 1), *(instructionPtr + 2));
		else
			printf("%04X %02X%02X        ", logicalStartAddr, opCode, *(instructionPtr + 1));
		break;
	case ADDR_IMMEDIATE_WORD:
	case ADDR_INDEXED_WORD:
	case ADDR_RELATIVE_WORD:
	case ADDR_EXTENDED:
		if ((curOpCode == 0x10) || (curOpCode == 0x11))
			printf("%04X %02X%02X%02X%02X    ", logicalStartAddr, opCode, *(instructionPtr + 1), *(instructionPtr + 2), *(instructionPtr + 3));
		else
			printf("%04X %02X%02X%02X      ", logicalStartAddr, opCode, *(instructionPtr + 1), *(instructionPtr + 2));
		break;
	case ADDR_INDEXED_DWORD:
		if ((curOpCode == 0x10) || (curOpCode == 0x11))
			printf("%04X %02X%02X%02X%02X%02X    ", logicalStartAddr, opCode, *(instructionPtr + 1), *(instructionPtr + 2), *(instructionPtr + 3), *(instructionPtr + 4));
		else
			printf("%04X %02X%02X%02X%02X    ", logicalStartAddr, opCode, *(instructionPtr + 1), *(instructionPtr + 2), *(instructionPtr + 3));
		break;
	}

}

void Cpu6809::manuallySetPC(uint16_t newPC)
{
	cpuReg.PC = newPC;
}