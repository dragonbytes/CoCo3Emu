#include "GimeBus.h"
#include "CoCo3EmuPGE.h"
#include <conio.h>
#include <ctime>

GimeBus::GimeBus()
{
	// Setup initial RAM state
	std::srand(std::time(0));								// Seed the pseudo-random number generator
	SetRAMSize(128);										// Emulator should default to stock CoCo 3 which has 128KB RAM

	offscreenBuffer.resize(640 * 480 * sizeof(olc::Pixel));

	cpu.ConnectBus(this);
	
	// Setup GIME registers initial states
	gimeAllRamModeEnabled = false;							// Initial state has ROM mapped into memory
	// Init 0 setup
	gimeRegInit0.cocoCompatMode = false;
	gimeRegInit0.mmuEnabled = false;
	gimeRegInit0.chipIRQEnabled = false;
	gimeRegInit0.chipFIRQEnabled = false;
	gimeRegInit0.constSecondaryVectors = false;
	gimeRegInit0.scsEnabled = false;
	gimeRegInit0.romMapControl = 0x00;
	// Init 1 setup
	gimeRegInit1.memoryType = false;
	gimeRegInit1.timerSource = false;						// False = 63.695 usec
	gimeRegInit1.mmuTaskSelect = 0x00;						// 0 = 0xFFA0-0xFFA7 (Task 0)

	samPageSelectReg = 0x00;

	// One CPU clock occurs for every 4 NTSC video color burst clock cycles, and one video color burst cycle occurs for every 8 (GIME) Master Clock cycles.
	// CoCo 3's power-up clock speed is ~0.89 MHz (slow-mode), so with the cycle scaling described above, 1 (slow-mode) CPU clock tick happens every 32 GIME Master clock cycles
	cpuClockDivisor = 8;
	masterBusCycleCount = 0;
	curScanline = 0;
	dotCounter = 0;
}

void GimeBus::SetRAMSize(int sizeInKB)
{
	ramTotalSizeKB = sizeInKB;
	physicalRAM.resize(sizeInKB * 1024);
	printf("CoCo 3 RAM Size set to %u bytes.\n", (unsigned int)physicalRAM.size());
	// Init all the physical RAM to random values which is what happens on real hardware
	//for (int i = 0; i < (sizeInKB * 1024); i++)
	//	physicalRAM[i] = std::rand() % 256;
	uint8_t blankByte = 0x00;
	for (uint32_t i = 0; i < (sizeInKB * 1024); i++)
	{
		if ((i % 4) == 0)
			blankByte = ~blankByte;
		physicalRAM[i] = blankByte;
	}
}

void GimeBus::gimeBusClockTick()
{
	// Check if we are at falling edge of Field Sync which occurs at scanline dot 1422 on scanline 230
	if ((dotCounter == 356) && (curScanline == 230))
	{
		devPIA0.SideB.controlReg |= 0x80;
		if (!(devPIA0.SideB.controlReg & 0x02) && (devPIA0.SideB.controlReg & 0x01))
			cpu.assertedInterrupts[INT_IRQ] = IRQ_SRC_PIA_VSYNC;
	}
	dotCounter++;
	//masterBusCycleCount++;
		
	if ((dotCounter % 8) == 0)		// In normal speed, cpuCLockDivisor = 4 and so every 4th GIME cycle, we increment the CPU cycle by 1
		cpu.cpuClockTick();

	/*
	if (dotCounter == 168)
	{
		
	}
	else if (dotCounter == 1820)
	{
		dotCounter = 0;
		curScanline++;
		if (curScanline == 262)
			curScanline = 0;
	}
	*/
	return;
}

void GimeBus::buildNewScanline(uint16_t scanlineNum)
{

}

void GimeBus::updateVideoParams()
{
	if (ramTotalSizeKB > 128)
		videoStartAddr = ((gimeVertOffsetMSB >> 5) * 0x10000) + (samPageSelectReg * 512) + ((gimeVertOffsetLSB & 0x3F) * 8);
	else
		videoStartAddr = (((gimeVertOffsetMSB >> 5) * 0x10000) + (samPageSelectReg * 512) + ((gimeVertOffsetLSB & 0x3F) * 8)) & 0x1FFFF;
}

uint8_t GimeBus::readPhysicalByte(uint16_t address)
{
	// This function's purpose is to handle the different hardware configs/states to "translate" an address to actual allocated "Physical" RAM
	uint32_t destPhysicalAddr;
	uint8_t mmuRegisterIndex;

	// Check if requested address in within the Secondary Vectors range of 0xFE00-0xFEFF. If so, check GIME flag as to whether the corresponding
	// Physical Address is constantly mapped directly to 0x70000 or if the MMU block mapping applies instead. Then do the things.
	if (gimeRegInit0.mmuEnabled && ((address < 0xFE00) || !gimeRegInit0.constSecondaryVectors))
	{
		// Reduce the 16-bit Logical address to number of 2K blocks to use as index into the MMU Bank registers. If MMU Task 1 is set,
		// advance the index by 8 bytes to point to second set of banks. Keep as-is if Task 0 is set.
		mmuRegisterIndex = (address >> 13) + (gimeRegInit1.mmuTaskSelect * 8);
		// Now use that index to get the Physical Memory start address of corresponding MMU Block number, then reduce our Logical Address to range 0x0000-0x1FFF because
		// each MMU block is 0x2000 bytes in size. Then add that modified address offset to our Physical address to get the final address we need.
		destPhysicalAddr = gimeMMUBankRegs[mmuRegisterIndex].mmuBlockAddr + (address & 0x1FFF);
	}
	else
		destPhysicalAddr = address + 0x70000;	// If MMU is disabled or if accessing "Constant" Secondary vectors, ALL Logical addresses get directly mapped to 0x70000 region of Physical RAM

	if (ramTotalSizeKB > 128)
		return physicalRAM[destPhysicalAddr];
	else
		return physicalRAM[destPhysicalAddr & 0x1FFFF];
}

uint8_t GimeBus::writePhysicalByte(uint16_t address, uint8_t byte)
{
	uint32_t destPhysicalAddr;
	uint8_t mmuRegisterIndex;

	if (gimeRegInit0.mmuEnabled && ((address < 0xFE00) || !gimeRegInit0.constSecondaryVectors))
	{
		// Reduce the 16-bit Logical address to number of 2K blocks to use as index into the MMU Bank registers. If MMU Task 1 is set,
		// advance the index by 8 bytes to point to second set of banks. Keep as-is if Task 0 is set.
		mmuRegisterIndex = (address >> 13) + (gimeRegInit1.mmuTaskSelect * 8);
		// Now use that index to get the Physical Memory start address of corresponding MMU Block number, then reduce our Logical Address to range 0x0000-0x1FFF because
		// each MMU block is 0x2000 bytes in size. Then add that modified address offset to our Physical address to get the final address we need.
		destPhysicalAddr = gimeMMUBankRegs[mmuRegisterIndex].mmuBlockAddr + (address & 0x1FFF);
	}
	else
		destPhysicalAddr = address + 0x70000;	// If MMU is disabled or if accessing "Constant" Secondary vectors, ALL Logical addresses get directly mapped to 0x70000 region of Physical RAM

	if (ramTotalSizeKB > 128)
		physicalRAM[destPhysicalAddr] = byte;
	else
		physicalRAM[destPhysicalAddr & 0x1FFFF] = byte;
	
	return byte;
}

uint8_t GimeBus::readMemoryByte(uint16_t address)
{
	// These sequenuces of IF statements determine with or not the requested address maps to hardware I/O, ROM code, CPU Vectors, or actual RAM
	if (address >= 0xFFE0)
	{
		if (romCoCo3 != nullptr)
			return romCoCo3->readByte(address);		// CPU Vector region always maps to end of 32k CoCo 3 internal ROM
	}
	else if (address >= 0xFF00)
	{
		switch (address)
		{
		case 0xFF00:
			//printf("Read from $FF00\n"); 
			if (!(devPIA0.SideA.controlReg & PIA_CTRL_DIR_MASK))
				return devPIA0.SideA.dataDirReg;
			else if (devPIA0.SideA.dataDirReg == 0x00)
				return (getCocoKey(devPIA0.SideB.dataReg));
			else
				return (devPIA0.SideA.dataReg);
		case 0xFF01:
			return devPIA0.SideA.controlReg;
		case 0xFF02:
			devPIA0.SideB.controlReg &= 0x7F;
			if (cpu.assertedInterrupts[INT_IRQ] == IRQ_SRC_PIA_VSYNC)
				cpu.assertedInterrupts[INT_IRQ] = 0;
			if (!(devPIA0.SideB.controlReg & PIA_CTRL_DIR_MASK))
				return devPIA0.SideB.dataDirReg;
			else if (devPIA0.SideB.dataDirReg == 0x00)
				return (getCocoKey(devPIA0.SideA.dataReg));
			else
				return (devPIA0.SideB.dataReg);
		case 0xFF03:
			return devPIA0.SideB.controlReg;
		default:
			return 0xFF;
		}
	}
	else if (gimeAllRamModeEnabled || (address < 0x8000))
	{
		// This section is relevant if either the requested address is below the ROM regions (definetly RAM) OR if ROM-mode is disabled and ALL regions are RAM
		return readPhysicalByte(address);
	}
	else if (gimeRegInit0.romMapControl == GIME_ROM_32_INTERNAL)
	{
		// If here, we are in a ROM-mapped mode. Figure out if the ROM(s) is internal and/or external, and how large it/they are
		if (romCoCo3 != nullptr)
			return romCoCo3->readByte(address);
		else
			return 0x1B;	// DEBUG: just for now, to mimic XRoar. Should ACTUALLY be the last value on the CPU's data bus
	}
	else if (gimeRegInit0.romMapControl < GIME_ROM_32_INTERNAL)
	{
		// If here, we are in 16K internal plus 16K external ROM mapping mode
		if (address >= 0xC000)
			if (romExternal != nullptr)
				return romExternal->readByte(address);
			else
				return 0x1B;	// DEBUG: just for now, to mimic XRoar. Should ACTUALLY be the last value on the CPU's data bus
		else if (address >= 0x8000)
			if (romCoCo3 != nullptr)
				return romCoCo3->readByte(address);
			else
				return 0x1B;	// DEBUG: just for now, to mimic XRoar. Should ACTUALLY be the last value on the CPU's data bus
	}
}

uint8_t GimeBus::writeMemoryByte(uint16_t address, uint8_t byte)
{
	if (address >= 0xFF00)
	{
		// Check GIME MMU Bank Registers
		if (gimeRegInit0.mmuEnabled && (address >= 0xFFA0) && (address <= 0xFFAF))
		{
			gimeMMUBankRegs[address & 0x000F].bankNum = byte;
			gimeMMUBankRegs[address & 0x000F].mmuBlockAddr = byte * 0x2000;
		}
		else if ((address >= 0xFFB0) && (address <= 0xFFBF))
		{
			// GIME Palette Registers
			gimePaletteRegs[address & 0x000F] = (byte & 0x3F);		// Use mask to enforce color is within GIME's 64 color range
		}
		else if ((address >= 0xFFC6) && (address <= 0xFFD3))
		{
			if (address & 0x0001)
				samPageSelectReg |= samPageSelectMasks[(address - 0xFFC6)];
			else
				samPageSelectReg &= samPageSelectMasks[(address - 0xFFC6)];
			updateVideoParams();
		}
		else
		{
			switch (address)
			{
			case 0xFF00:		// PIA0 Side A Data/Direction Register
				if (!(devPIA0.SideA.controlReg & PIA_CTRL_DIR_MASK))
					devPIA0.SideA.dataDirReg = byte;
				else
				{	// Now we use masking to make sure that only the bits set as "Outputs" in data 
					// direction register receive actual data from the written byte
					devPIA0.SideA.dataReg &= ~devPIA0.SideA.dataDirReg;
					devPIA0.SideA.dataReg |= (byte & devPIA0.SideA.dataDirReg);
				}
				break;
			case 0xFF01:		// PIA0 Side A Control Register
				devPIA0.SideA.controlReg = (byte & 0b00111111) | 0b00110000;
				break;
			case 0xFF02:		// PIA0 Side B Data/Direction Register
				//printf("Wrote to $FF02\n");
				if (!(devPIA0.SideB.controlReg & PIA_CTRL_DIR_MASK))
					devPIA0.SideB.dataDirReg = byte;
				else
				{
					// Now we use masking to make sure that only the bits set as Outputs in data register 
					// receive actual data from the written byte
					devPIA0.SideB.dataReg &= ~devPIA0.SideB.dataDirReg;
					devPIA0.SideB.dataReg |= (byte & devPIA0.SideB.dataDirReg);
				}
				break;
			case 0xFF03:		// PIA0 Side B Control Register
				devPIA0.SideB.controlReg = (devPIA0.SideB.controlReg & 0x80) | ((byte & 0x0F) | 0b00110000);
				break;
				// GIME Hardware Registers
			case 0xFF90:		// GIME Initialization Register 0 (INIT0)
				gimeRegInit0.cocoCompatMode = (byte & 0x80);
				gimeRegInit0.mmuEnabled = (byte & 0x40);
				gimeRegInit0.chipIRQEnabled = (byte & 0x20);
				gimeRegInit0.chipFIRQEnabled = (byte & 0x10);
				gimeRegInit0.constSecondaryVectors = (byte & 0x08);
				gimeRegInit0.scsEnabled = (byte & 0x04);
				gimeRegInit0.romMapControl = (byte & 0b00000011);
				break;
			case 0xFF9D:
				gimeVertOffsetMSB = byte;
				break;
			case 0xFF9E:
				gimeVertOffsetLSB = byte;
				break;
			case 0xFFDE:
			case 0xFFDF:
				gimeAllRamModeEnabled = (address & 0x0001);
				break;
			}
		}
	}
	// If here, the Logical address is less than 0xFF00
	else if (gimeAllRamModeEnabled || (address < 0x8000))
	{
		writePhysicalByte(address, byte);
	}
	return byte;		// This is just a placeholder in case later, for hardware register-related reasons, I want to indicate result of writing to I/O
}

uint16_t GimeBus::readMemoryWord(uint16_t address)
{
	return ((readMemoryByte(address) * 256) + readMemoryByte(address + 1));
}

uint16_t GimeBus::writeMemoryWord(uint16_t address, uint16_t word)
{
	writeMemoryByte(address, (word & 0xFF00) >> 8);
	writeMemoryByte(address + 1, word & 0x00FF);
	return word;		// This is just a placeholder in case later, for hardware register-related reasons, I want to indicate result of writing to I/O
}

uint8_t GimeBus::getCocoKey(uint8_t columnsByte)
{
	uint8_t rowByte = 0xFF;
	uint8_t matrixOffset = 56 - 7;

	columnsByte = ~columnsByte;
	while (columnsByte)
	//for (int i=0; i < 8; i++)
	{
		if (columnsByte & 0x01)
			for (int row = 0; row < 7; row++)
				if (hostKeyMatrix[matrixOffset + row].keyDownState)
					rowByte &= cocoKeyStrobeResult[row];		
		columnsByte >>= 1;
		matrixOffset -= 7;
	}
	return rowByte;
}
