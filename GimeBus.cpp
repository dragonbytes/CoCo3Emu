#include "GimeBus.h"
#include "CoCo3EmuPGE.h"
#include <conio.h>
#include <ctime>

GimeBus::GimeBus()
{
	// Setup initial RAM state
	std::srand(std::time(0));								// Seed the pseudo-random number generator
	SetRAMSize(512);										// Emulator should default to stock CoCo 3 which has 128KB RAM

	//offscreenBuffer.resize(640 * 480 * sizeof(olc::Pixel));

	cpu.ConnectToBus(this);
	diskController.ConnectToBus(this);
	emuDiskDriver.ConnectToBus(this);
	//serial.ConnectToBus(this);

	// Joysticks state setup
	joystickDevice[JOYSTICK_PORT_RIGHT].portName = "RIGHT";
	joystickDevice[JOYSTICK_PORT_LEFT].portName = "LEFT";
	
	// Setup GIME registers initial states
	gimeAllRamModeEnabled = false;							// Initial state has ROM mapped into memory
	// Init 0 setup
	gimeRegInit0.cocoCompatMode = false;
	gimeRegInit0.mmuEnabled = false;
	gimeRegInit0.chipIRQEnabled = false;					// Default to GIME IRQs being disabled
	gimeRegInit0.chipFIRQEnabled = false;					// Default to GIME FIRQs being disabled
	gimeRegInit0.constSecondaryVectors = false;
	gimeRegInit0.scsEnabled = false;
	gimeRegInit0.romMapControl = 0x00;
	// Init 1 setup
	gimeRegInit1.memoryType = false;
	gimeRegInit1.timerSourceFast = false;					// False = 63.695 usec
	gimeRegInit1.mmuTaskSelect = 0x00;						// 0 = 0xFFA0-0xFFA7 (Task 0)

	gimeHorizontalOffsetReg.offsetAddress = 0;
	gimeHorizontalOffsetReg.hven = false;

	gimeBlinkStateOn = false;

	gimeRegIRQtypes = 0x00;
	gimeRegFIRQtypes = 0x00;

	samPageSelectReg = 0x00;
	vdgVideoConfig.gfxModeEnabled = false;
	vdgVideoConfig.fontIsExternal = false;

	// One CPU clock occurs for every 4 NTSC video color burst clock cycles, and one video color burst cycle occurs for every 8 (GIME) Master Clock cycles.
	// CoCo 3's power-up clock speed is ~0.89 MHz (slow-mode), so with the cycle scaling described above, 1 (slow-mode) CPU clock tick happens every 32 GIME Master clock cycles
	cpuClockDivisor = 32;
	masterBusCycleCounter = 0;
	scanlineCounter = 0;
	dotCounter = 0;
	floppySeekIntervalCounter = 0;
	floppyAccessIntervalCounter = 0;

	emuInfoTextIndex = -3;
}

void GimeBus::SetRAMSize(int sizeInKB)
{
	ramTotalSizeKB = sizeInKB;
	ramSizeMask = (sizeInKB * 1024) - 1;
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
	if ((dotCounter == 1422) && (scanlineCounter == 230))
	{
		devPIA0.SideB.controlReg |= 0x80;
		if ((devPIA0.SideB.controlReg & PIA_CTRL_IRQENABLE_MASK) && !(devPIA0.SideB.controlReg & PIA_CTRL_IRQEDGE_MASK))
			cpu.assertedInterrupts[INT_IRQ] |= INT_ASSERT_MASK_PIA_VSYNC;
	}
	dotCounter++;
	masterBusCycleCounter++;

	if (diskController.isConnected)
	{
		// Check if the emulated disk is spinning, and if so, emulate the state of the Index Pulse
		if (diskController.fdcMotorOn)
		{
			floppyIndexHoleCounter++;
			// Set the internal Floppy Index Pulse state if we are at the right points in the counter/rotation
			if (floppyIndexHoleCounter >= gimePerFloppyRotation)
			{
				diskController.fdcIndexPulse = false;
				floppyIndexHoleCounter = 0;		// If the emulated floppy has completed one complete rotation (assuming 300 rpm), then reset our counter until next Index Pulse will occur
			}
			else if (floppyIndexHoleCounter >= (gimePerFloppyRotation - gimeFloppyIndexWidth))
				diskController.fdcIndexPulse = true;
		}

		if (diskController.fdcPendingCommand != FDC_OP_NONE)
		{
			// Check if we are processing a "Type I" floppy head-stepping related command and handle it if so
			if ((diskController.fdcPendingCommand >= FDC_OP_RESTORE) && (diskController.fdcPendingCommand <= FDC_OP_STEP_OUT))
			{
				// Copy internal Floppy Index Pulse state from internal variable to FDC Status Register (Bit 1)
				if (diskController.fdcIndexPulse)
					diskController.fdcStatusReg |= FDC_STATUS_I_INDEX;
				else
					diskController.fdcStatusReg &= ~FDC_STATUS_I_INDEX;

				floppySeekIntervalCounter++;
				if (floppySeekIntervalCounter > 28636)
				{
					// 28636 GIME cycles approximately works out to be 1 millisecond in real-time, which we can use to time our floppy head-stepping rate
					floppySeekIntervalCounter = 0;
					if (diskController.fdcHandleNextEvent() == FD502_OPERATION_COMPLETE)
					{
						diskController.fdcHaltFlag = false;
						if (diskController.fdcDoubleDensity)
							cpu.assertedInterrupts[INT_NMI] |= INT_ASSERT_MASK_NMI;		// NMI is only asserted when the controller is set for Double Density operation for some reason
					}
				}
			}
			// If we are processing a "Type II or III" command and active floppy access is happening, handle the next corresponding floppy event
			else if (diskController.fdcMotorOn && (diskController.fdcPendingCommand >= FDC_OP_READ_SECTOR) && (diskController.fdcPendingCommand <= FDC_OP_WRITE_TRACK))
			{
				floppyAccessIntervalCounter++;
				if (!diskController.fdcHeadLoaded && (floppyAccessIntervalCounter > (28636 * 100)))
				{
					floppyAccessIntervalCounter = 0;
					floppyFormatIndexCounter = 0;
					diskController.fdcHeadLoaded = true;
					if (diskController.fdcPendingCommand == FDC_OP_WRITE_SECTOR)
					{
						diskController.fdcStatusReg |= FDC_STATUS_II_III_DATA_REQUEST;
						if (diskController.fdcHaltFlag)
						{
							cpu.cpuHardwareHalt = false;
							cpu.cpuHaltAsserted = false;
						}
					}
				}
				else if (diskController.fdcHeadLoaded && (floppyAccessIntervalCounter > 916))
				{
					// 916 GIME Master Bus cycles works out to be approximately 32 microseconds (it's technically 916.3635200000006) which is the data rate defined in the docs for MFM (double density)
					floppyAccessIntervalCounter = 0;
					uint8_t fdcEventResult = diskController.fdcHandleNextEvent();
					if (fdcEventResult == FD502_OPERATION_COMPLETE)
					{
						if (diskController.fdcDoubleDensity)
							cpu.assertedInterrupts[INT_NMI] |= INT_ASSERT_MASK_NMI;
					}
				}
			}
		}
	}

	if (dotCounter == 1820)
	{
		dotCounter = 0;
		scanlineCounter++;
		if (scanlineCounter == 262)
		{
			scanlineCounter = 0;
			if (devPIA0.SideB.controlReg & PIA_CTRL_IRQEDGE_MASK)
			{
				//devPIA0.SideB.controlReg |= 0x80;
				if (devPIA0.SideB.controlReg & PIA_CTRL_IRQENABLE_MASK)
					cpu.assertedInterrupts[INT_IRQ] |= INT_ASSERT_MASK_PIA_VSYNC;
			}
		}
		else if (scanlineCounter == 13)
		{
			if ((gimeRegIRQtypes & GIME_INT_MASK_VBORDER) && gimeRegInit0.chipIRQEnabled)
			{
				gimeIRQstatus |= GIME_INT_MASK_VBORDER;
				cpu.assertedInterrupts[INT_IRQ] |= INT_ASSERT_MASK_GIME;
			}
			if ((gimeRegFIRQtypes & GIME_INT_MASK_VBORDER) && gimeRegInit0.chipFIRQEnabled)
			{
				gimeFIRQstatus |= GIME_INT_MASK_VBORDER;
				cpu.assertedInterrupts[INT_FIRQ] |= INT_ASSERT_MASK_GIME;
			}
		}

		if (!gimeRegInit1.timerSourceFast && (gimeRegTimer.word != 0))
			decrementGimeTimer();							// The math says this should actually happen after 1824 GIME Master Clock cycles instead of 1820, but I have to double-check...
	}

	// Check if GIME Timer is set to fastest interval which occurs every 8 GIME Master Clock cycles (NTSC Color Reference Frequency)
	// If both are true and the Timer is not disabled from being set to 0 by the user, decerement the GIME timer counter
	if (gimeRegInit1.timerSourceFast && (gimeRegTimer.word !=0) && ((masterBusCycleCounter % 8) == 0))
		decrementGimeTimer();

	if ((masterBusCycleCounter % cpuClockDivisor) == 0)
	{
		//masterBusCycleCounter = 0;
		cpu.cpuClockTick();
	}

	return;
}

void GimeBus::decrementGimeTimer()
{
	gimeTimerCounter--;
	if (gimeTimerCounter == 0)
	{
		gimeTimerCounter = gimeRegTimer.word;		// Reset the counter to it's set start value
		gimeBlinkStateOn = !gimeBlinkStateOn;		// Invert the on/off state for blinking text in the GIME Hardware Font text modes since it's governed by the Timer
		// TODO: Check if GIME registers have interrupts enabled for when timer hits zero
		if ((gimeRegIRQtypes & GIME_INT_MASK_TIMER) && gimeRegInit0.chipIRQEnabled)
		{
			cpu.assertedInterrupts[INT_IRQ] |= INT_ASSERT_MASK_GIME;
			gimeIRQstatus |= GIME_INT_MASK_TIMER;	// This sets the relevant flag in our GIME IRQ "status" variable so when IRQENR register is read, interrupt source can be determined and acknowledged
		}
		if ((gimeRegFIRQtypes & GIME_INT_MASK_TIMER) && gimeRegInit0.chipFIRQEnabled)
		{
			cpu.assertedInterrupts[INT_FIRQ] |= INT_ASSERT_MASK_GIME;
			gimeFIRQstatus |= GIME_INT_MASK_TIMER;	// This sets the relevant flag in our GIME FIRQ "status" variable so when FIRQENR register is read, interrupt source can be determined and acknowledged
		}
	}
}

void GimeBus::updateVideoParams()
{
	if (gimeRegInit0.cocoCompatMode)
	{
		videoStartAddr = ((gimeVertOffsetMSB >> 5) * 0x10000) + (samPageSelectReg * 512) + ((gimeVertOffsetLSB & 0x3F) * 8) + gimeHorizontalOffsetReg.offsetAddress;
		vdgVideoConfig.gfxModeEnabled = devPIA1.SideB.dataReg & 0x80;
		vdgVideoConfig.colorSetSelect = (devPIA1.SideB.dataReg & 0b00001000) >> 3;
		curResolutionWidth = 512;
		curResolutionHeight = 192;

		if (vdgVideoConfig.gfxModeEnabled)
		{
			uint8_t videoMode = ((devPIA1.SideB.dataReg & 0b01110000) >> 1) | samVideoDisplayReg;
			switch (videoMode)
			{
			case 0b00000001:
				vdgVideoConfig.colorDepth = 4;
				mainPtr->bytesPerPixelRow = 16;
				mainPtr->pixelsPerDraw = 8;
				break;
			case 0b00001001:
				vdgVideoConfig.colorDepth = 2;
				mainPtr->bytesPerPixelRow = 16;
				mainPtr->pixelsPerDraw = 4;
				break;
			case 0b00010010:
				vdgVideoConfig.colorDepth = 4;
				mainPtr->bytesPerPixelRow = 32;
				mainPtr->pixelsPerDraw = 4;
				break;
			case 0b00011011:
				vdgVideoConfig.colorDepth = 2;
				mainPtr->bytesPerPixelRow = 16;
				mainPtr->pixelsPerDraw = 4;
				break;
			case 0b00100100:
				vdgVideoConfig.colorDepth = 4;
				mainPtr->bytesPerPixelRow = 32;
				mainPtr->pixelsPerDraw = 4;
				break;
			case 0b00101101:
				vdgVideoConfig.colorDepth = 2;
				mainPtr->bytesPerPixelRow = 16;
				mainPtr->pixelsPerDraw = 4;
				break;
			case 0b00110110:
				vdgVideoConfig.colorDepth = 4;
				mainPtr->bytesPerPixelRow = 32;
				mainPtr->pixelsPerDraw = 4;
				break;
			case 0b00111110:
				vdgVideoConfig.colorDepth = 2;
				mainPtr->bytesPerPixelRow = 32;
				mainPtr->pixelsPerDraw = 2;
				break;
			}
			mainPtr->colorSetOffset = vdgVideoConfig.colorDepth * vdgVideoConfig.colorSetSelect;
			mainPtr->borderPixel = vdgBorderPaletteDefs[vdgVideoConfig.colorSetSelect];
		}
		else 
			vdgVideoConfig.fontIsExternal = devPIA1.SideB.dataReg & 0x10;
	}
	else
	{
		videoStartAddr = (gimeVertOffsetMSB * 2048) + (gimeVertOffsetLSB * 8) + gimeHorizontalOffsetReg.offsetAddress;
		curResolutionHeight = gimeVerticalResolutions[gimeRegVRES.LPF];
		if (!gimeRegVMode.gfxOrTextMode)
		{
			// Text display mode
			mainPtr->curLinesPerRow = gimeLinesPerRow[gimeRegVMode.LPR];
			mainPtr->underlineRowOffset = gimeUnderlineOffset[mainPtr->curLinesPerRow - 8];
			mainPtr->curBytesPerChar = (gimeRegVRES.CRES & 0x01) + 1;	// If color attributes are enabled, will be 2 bytes/char, otherwise 1
			mainPtr->curCharsPerRow = textCharsPerRow[gimeRegVRES.HRES];
			mainPtr->curBytesPerCharRow = mainPtr->curCharsPerRow * mainPtr->curBytesPerChar;
			curResolutionWidth = gimeHorizontalResolutions[((gimeRegVRES.HRES & 0x05) >> 1) | (gimeRegVRES.HRES & 0x01)];
		}
		else
		{
			// GIME Graphics Modes
			mainPtr->curLinesPerRow = gimeLinesPerRow[gimeRegVMode.LPR];
			mainPtr->bytesPerPixelRow = gfxBytesPerRow[gimeRegVRES.HRES];

			switch ((gimeRegVRES.HRES << 2) | gimeRegVRES.CRES)
			{
			case 0b00011101:
			case 0b00010100:
				mainPtr->pixelsPerDraw = 1;
				curResolutionWidth = 640;
				break;
			case 0b00011001:
			case 0b00010000:
				mainPtr->pixelsPerDraw = 1;
				curResolutionWidth = 512;
				break;
			case 0b00011110:
			case 0b00010101:
			case 0b00001100:
				mainPtr->pixelsPerDraw = 2;
				curResolutionWidth = 640;
				break;
			case 0b00011010:
			case 0b00010001:
			case 0b00001000:
				mainPtr->pixelsPerDraw = 2;
				curResolutionWidth = 512;
				break;
			case 0b00010110:
			case 0b00001101:
			case 0b00000100:
				mainPtr->pixelsPerDraw = 4;
				curResolutionWidth = 640;
				break;
			case 0b00010010:
			case 0b00001001:
			case 0b00000000:
				mainPtr->pixelsPerDraw = 4;
				curResolutionWidth = 512;
				break;
			}
		}
	}

	// If total system RAM is stock CoCo 3 128K, mask off the upper bits to keep within the limited physical address space
	if (ramTotalSizeKB == 128)
		videoStartAddr &= 0x1FFFF;
}

uint8_t GimeBus::readPhysicalByte(uint16_t address)
{
	// This function's purpose is to handle the different hardware configs/states to "translate" an address to actual allocated "Physical" RAM
	uint32_t destPhysicalAddr;
	// Check if requested address in within the Secondary Vectors range of 0xFE00-0xFEFF. If so, check GIME flag as to whether the corresponding
	// Physical Address is constantly mapped directly to 0x70000 or if the MMU block mapping applies instead. Then do the things.
	if (gimeRegInit0.mmuEnabled && ((address < 0xFE00) || !gimeRegInit0.constSecondaryVectors))
	{
		// Reduce the 16-bit Logical address to number of 2K blocks to use as index into the MMU Bank registers. If MMU Task 1 is set,
		// advance the index by 8 bytes to point to second set of banks. Keep as-is if Task 0 is set.
		uint8_t mmuRegisterIndex = (address >> 13) + (gimeRegInit1.mmuTaskSelect * 8);
		if (!gimeAllRamModeEnabled && ((gimeMMUBankRegs[mmuRegisterIndex].bankNum >= 0x3C) && (gimeMMUBankRegs[mmuRegisterIndex].bankNum <= 0x3F)))
			return readByteFromROM(address);
		// Now use that index to get the Physical Memory start address of corresponding MMU Block number, then reduce our Logical Address to range 0x0000-0x1FFF because
		// each MMU block is 0x2000 bytes in size. Then add that modified address offset to our Physical address to get the final address we need.
		destPhysicalAddr = gimeMMUBankRegs[mmuRegisterIndex].mmuBlockAddr + (address & 0x1FFF);
	}
	else
	{
		if (!gimeAllRamModeEnabled && !gimeRegInit0.mmuEnabled && (address >= 0x8000))
			return readByteFromROM(address);
		destPhysicalAddr = address + 0x70000;	// If MMU is disabled or if accessing "Constant" Secondary vectors, ALL Logical addresses get directly mapped to 0x70000 region of Physical RAM
	}

	// If here, the target physical address is indeed in RAM
	return physicalRAM[destPhysicalAddr & ramSizeMask];
}

uint8_t GimeBus::writePhysicalByte(uint16_t address, uint8_t byte)
{
	uint32_t destPhysicalAddr;
	if (gimeRegInit0.mmuEnabled && ((address < 0xFE00) || !gimeRegInit0.constSecondaryVectors))
	{
		// Reduce the 16-bit Logical address to number of 2K blocks to use as index into the MMU Bank registers. If MMU Task 1 is set,
		// advance the index by 8 bytes to point to second set of banks. Keep as-is if Task 0 is set.
		uint8_t mmuRegisterIndex = (address >> 13) + (gimeRegInit1.mmuTaskSelect * 8);
		if (!gimeAllRamModeEnabled && ((gimeMMUBankRegs[mmuRegisterIndex].bankNum >= 0x3C) && (gimeMMUBankRegs[mmuRegisterIndex].bankNum <= 0x3F)))
			return byte;		// Tried writing to region that ROM has control over. Just return the entry byte and do nothing
		// Now use that index to get the Physical Memory start address of corresponding MMU Block number, then reduce our Logical Address to range 0x0000-0x1FFF because
		// each MMU block is 0x2000 bytes in size. Then add that modified address offset to our Physical address to get the final address we need.
		destPhysicalAddr = gimeMMUBankRegs[mmuRegisterIndex].mmuBlockAddr + (address & 0x1FFF);
	}
	else
	{
		if (!gimeAllRamModeEnabled && !gimeRegInit0.mmuEnabled && (address >= 0x8000))
			return byte;		// Tried writing to region that ROM has control over. Just return the entry byte and do nothing
		destPhysicalAddr = address + 0x70000;	// If MMU is disabled or if accessing "Constant" Secondary vectors, ALL Logical addresses get directly mapped to 0x70000 region of Physical RAM
	}

	physicalRAM[destPhysicalAddr & ramSizeMask] = byte;
	
	return byte;
}

uint8_t GimeBus::readByteFromROM(uint16_t address)
{
	switch (gimeRegInit0.romMapControl)
	{
	case GIME_ROM_16_SPLIT_0:
	case GIME_ROM_16_SPLIT_1:
		// ROM 16K Internal + 16K External mode
		if (((address & 0x7FFF) >= 0x4000) && (romExternal != nullptr))
			return romExternal->readByte(address);
		else if (romCoCo3 != nullptr)
			return romCoCo3->readByte(address);
		break;
	case GIME_ROM_32_INTERNAL:
		// ROM 32K Internal only
		if (romCoCo3 != nullptr)
			return romCoCo3->readByte(address);
		break;
	case GIME_ROM_32_EXTERNAL:
		// ROM 32k External only
		if (romExternal != nullptr)
			return romExternal->readByte(address);
		break;
	}
	return 0x1B;	// DEBUG: just for now, to mimic XRoar. Should ACTUALLY be the last value on the CPU's data bus
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
		// Check GIME MMU Bank Registers
		if ((address >= 0xFFA0) && (address <= 0xFFAF))
			return gimeMMUBankRegs[address & 0x000F].bankNum;

		if ((address >= 0xFFB0) && (address <= 0xFFBF))
			return gimePaletteRegs[address & 0x000F];

		switch (address)
		{
		case 0xFF00:
			if (!(devPIA0.SideA.controlReg & PIA_CTRL_DIR_MASK))
				return devPIA0.SideA.dataDirReg;
			joystickPortIndex = (devPIA0.SideB.controlReg & 0b00001000) >> 3;
			joystickAxisValue = (devPIA0.SideA.controlReg & 0b00001000) ? joystickDevice[joystickPortIndex].yAxis : joystickDevice[joystickPortIndex].xAxis;
			joystickCompareResult = (joystickAxisValue >= (devPIA1.SideA.dataReg >> 2)) ? 0x80 : 0x00;
			if (devPIA0.SideA.dataDirReg == 0x00)
				returnByte = ((getCocoKey(devPIA0.SideB.dataReg) & 0x7F) | joystickCompareResult);
			else
				returnByte = ((devPIA0.SideA.dataReg & 0x7F) | joystickCompareResult);
			if (joystickDevice[JOYSTICK_PORT_LEFT].buttonDown2)
				returnByte &= 0b11110111;
			if (joystickDevice[JOYSTICK_PORT_RIGHT].buttonDown2)
				returnByte &= 0b11111011;
			if (joystickDevice[JOYSTICK_PORT_LEFT].buttonDown1)
				returnByte &= 0b11111101;
			if (joystickDevice[JOYSTICK_PORT_RIGHT].buttonDown1)
				returnByte &= 0b11111110;
			return returnByte;
		case 0xFF01:
			return devPIA0.SideA.controlReg;
		case 0xFF02:
			devPIA0.SideB.controlReg &= 0x7F;
			// If an IRQ is currently being asserted by this PIA VSYNC pin, clear/acknowledge it
			if (cpu.assertedInterrupts[INT_IRQ] & INT_ASSERT_MASK_PIA_VSYNC)
				cpu.assertedInterrupts[INT_IRQ] &= ~INT_ASSERT_MASK_PIA_VSYNC;
			if (!(devPIA0.SideB.controlReg & PIA_CTRL_DIR_MASK))
				return devPIA0.SideB.dataDirReg;
			else if (devPIA0.SideB.dataDirReg == 0x00)
				return (getCocoKey(devPIA0.SideA.dataReg));
			else
				return (devPIA0.SideB.dataReg);
		case 0xFF03:
			return devPIA0.SideB.controlReg;
		// PIA1 Registers
		case 0xFF20:
			if (!(devPIA1.SideA.controlReg & PIA_CTRL_DIR_MASK))
				return devPIA1.SideA.dataDirReg;
			else
				return (devPIA1.SideA.dataReg);
			break;
		case 0xFF21:
			return devPIA1.SideA.controlReg;
		case 0xFF22:
			if (!(devPIA1.SideB.controlReg & PIA_CTRL_DIR_MASK))
				return devPIA1.SideB.dataDirReg;
			else
				return (devPIA1.SideB.dataReg);
		case 0xFF23:
			return devPIA1.SideB.controlReg;
		// Disk controller Registers
		case 0xFF48:
		case 0xFF49:
		case 0xFF4A:
		case 0xFF4B:
			if (diskController.isConnected)
				return diskController.fdcRegisterRead(address);
			break;
		// EmuDisk Virtual HD Registers
		case 0xFF80:
		case 0xFF81:
		case 0xFF82:
		case 0xFF84:
		case 0xFF85:
			if (emuDiskDriver.isEnabled)
				return 0x00;
			break;
		case 0xFF83:
			// Command/Status Register
			if (emuDiskDriver.isEnabled)
				return emuDiskDriver.statusCode;
			break;
		case 0xFF86:
			if (emuDiskDriver.isEnabled)
				return EMUDISK_STATUS_ERROR_INVALID_DRV_NUM;
			break;
		// GIME Registers
		case 0xFF92:
			// GIME IRQ Request Enable Register
			returnByte = gimeIRQstatus;
			gimeIRQstatus = 0x00;			// Clear/Acknowledge all active GIME IRQ interrupt sources
			// If IRQ is being asserted specifically by the GIME, then clear that particular source from our state variable
			if (cpu.assertedInterrupts[INT_IRQ] & INT_ASSERT_MASK_GIME)
			{
				cpu.assertedInterrupts[INT_IRQ] &= ~INT_ASSERT_MASK_GIME;
			}
			return returnByte;
		case 0xFF93:
			// Gime FIRQ Request Enable Register
			returnByte = gimeFIRQstatus;
			gimeFIRQstatus = 0x00;			// Clear/Acknowledge all active GIME FRQ interrupt sources
			// If FIRQ is being asserted specifically by the GIME, then clear that particular source from our state variable
			if (cpu.assertedInterrupts[INT_FIRQ] & INT_ASSERT_MASK_GIME)
				cpu.assertedInterrupts[INT_FIRQ] &= ~INT_ASSERT_MASK_GIME;
			return returnByte;
		// This is my custom idea for Emulator Info register. $FF96 supposedly isnt used by anything else real
		case 0xFF96:
			if (emuInfoTextIndex == -3)
			{
				returnByte = 'E';
				emuInfoTextIndex++;
			}
			else if (emuInfoTextIndex == -2)
			{
				returnByte = emuInfoVersionMajor;
				emuInfoTextIndex++;
			}
			else if (emuInfoTextIndex == -1)
			{
				returnByte = emuInfoVersionMinor;
				emuInfoTextIndex++;
			}
			else if (emuInfoTextIndex < emuInfoStringPtr->length())
			{
				// Since we are using a pointer, this ELSE IF statement should handle both Emulator Name and Extra Text strings
				returnByte = emuInfoStringPtr->at(emuInfoTextIndex);
				emuInfoTextIndex++;
			}
			// If here, we must have just finished returning one of the two possible strings. Figure out which and react accordingly
			else if (emuInfoStringPtr == &strEmuInfoName)
			{
				emuInfoStringPtr = &strEmuInfoExtra;
				emuInfoTextIndex = 0;
				returnByte = 0;
			}
			else if (emuInfoStringPtr == &strEmuInfoExtra)
			{
				emuInfoTextIndex = -3;		// This should loop back to the beginning of info from this special register
				emuInfoStringPtr = &strEmuInfoName;
				returnByte = 0;
			}
			return returnByte;
		default:
			return 0xFF;
		}
	}
	else
		return readPhysicalByte(address);
}

uint8_t GimeBus::writeMemoryByte(uint16_t address, uint8_t byte)
{
	if (address >= 0xFF00)
	{
		// Check GIME MMU Bank Registers
		if ((address >= 0xFFA0) && (address <= 0xFFAF))
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
		// Check for EmuDisk Virtual HD Control Registers
		else if ((address >= 0xFF80) && (address <= 0xFF86))
		{
			if (emuDiskDriver.isEnabled)
				emuDiskDriver.registerWrite(address, byte);
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
			case 0xFF20:
				if (!(devPIA1.SideA.controlReg & PIA_CTRL_DIR_MASK))
					devPIA1.SideA.dataDirReg = byte;
				else
				{	// Now we use masking to make sure that only the bits set as "Outputs" in data 
					// direction register receive actual data from the written byte
					devPIA1.SideA.dataReg &= ~devPIA1.SideA.dataDirReg;
					devPIA1.SideA.dataReg |= (byte & devPIA1.SideA.dataDirReg);
				}
				break;
			case 0xFF21:
				devPIA1.SideA.controlReg = (byte & 0b00111111) | 0b00110000;
				break;
			case 0xFF22:		// PIA1 Side B Data Register
				if (!(devPIA1.SideB.controlReg & PIA_CTRL_DIR_MASK))
					devPIA1.SideB.dataDirReg = byte;
				else
				{
					// Now we use masking to make sure that only the bits set as Outputs in data register 
					// receive actual data from the written byte
					uint8_t tempByte = devPIA1.SideB.dataReg;
					devPIA1.SideB.dataReg &= ~devPIA1.SideB.dataDirReg;
					devPIA1.SideB.dataReg |= (byte & devPIA1.SideB.dataDirReg);
					if ((tempByte & 0xF8) != (devPIA1.SideB.dataReg & 0xF8))
						updateVideoParams();			// Only update video mode parameters when video-related bits have changed 
				}
				break;
			case 0xFF23:		// PIA1 Side B Control Register
				devPIA1.SideB.controlReg = (byte & 0b00111111) | 0b00110000;
				break;
			// Floppy Disk Controller Registers
			case 0xFF40:
			case 0xFF48:
			case 0xFF49:
			case 0xFF4A:
			case 0xFF4B:
				if (diskController.isConnected)
					diskController.fdcRegisterWrite(address, byte);
				break;
			// Orchestra-90 Pak Registers
			case 0xFF7A:
				orch90dac.leftChannel = byte;
				break;
			case 0xFF7B:
				orch90dac.rightChannel = byte;
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
				updateVideoParams();
				break;
			case 0xFF91:
				gimeRegInit1.memoryType = (byte & 0x40);
				gimeRegInit1.timerSourceFast = (byte & 0x20);
				gimeRegInit1.mmuTaskSelect = (byte & 0x01);
				break;
			case 0xFF92:
				// GIME IRQ Enable Register
				gimeRegIRQtypes = (byte & 0x3F);
				break;
			case 0xFF93:
				// GIME FIRQ Enable Register
				gimeRegFIRQtypes = (byte & 0x3F);
				break;
			case 0xFF94:
				// GIME Timer Register MSB
				gimeRegTimer.MSB = (byte & 0x0F);			// GIME's timer is only 12-bit so mask off the upper 4 bits of most-significant byte
				if (gimeRegTimer.word != 0)
					gimeRegTimer.word += gimeTimerOffset;	// The timer is always processed with additional GIME cycles over what is set here, and it depends on the revision. HANDLE IT
				// Writing to FF94 always restarts the counter to the new value
				gimeTimerCounter = gimeRegTimer.word;
				break;
			case 0xFF95:
				// GIME Timer Register LSB
				gimeRegTimer.LSB = byte;
				if (gimeRegTimer.word != 0)
					gimeRegTimer.word += gimeTimerOffset;	// The timer is always processed with additional GIME cycles over what is set here, and it depends on the revision. HANDLE IT
				break;
			case 0xFF98:
				// Video Mode Register (VMODE)
				gimeRegVMode.gfxOrTextMode = (byte & 0x80);
				gimeRegVMode.cmpColorPhaseInvert = (byte & 0x20);
				gimeRegVMode.monoCompositeOut = (byte & 0x10);
				gimeRegVMode.video50hz = (byte & 0x08);
				gimeRegVMode.LPR = (byte & 0x07);
				updateVideoParams();
				break;
			case 0xFF99:
				// Video Resolution Register
				gimeRegVRES.LPF = (byte >> 5) & 0x03;
				gimeRegVRES.HRES = (byte >> 2) & 0x07;
				gimeRegVRES.CRES = (byte & 0x03);
 				updateVideoParams();
				break;
			case 0xFF9A:
				// Border Color Register
				gimeRegBorder = (byte & 0x3F);
				break;
			case 0xFF9D:
				gimeVertOffsetMSB = byte;
				updateVideoParams();
				break;
			case 0xFF9E:
				gimeVertOffsetLSB = byte;
				updateVideoParams();
				break;
			case 0xFF9F:
				gimeHorizontalOffsetReg.hven = (byte & 0x80);
				gimeHorizontalOffsetReg.offsetAddress = (byte & 0x7F);
				break;
			// SAM Video Display Registers
			// If changing the specified bits below will result in a different value than it's current value, perform the operation and call our updateVideoParams() function.
			case 0xFFC0:
			case 0xFFC2:
			case 0xFFC4:
				tempByte = samVideoDisplayReg;
				samVideoDisplayReg &= samVideoDisplayMasks[address & 0x0007];
				if (tempByte != samVideoDisplayReg)
					updateVideoParams();
				break;
			case 0xFFC1:
			case 0xFFC3:
			case 0xFFC5:
				tempByte = samVideoDisplayReg;
				samVideoDisplayReg |= samVideoDisplayMasks[address & 0x0007];
				if (tempByte != samVideoDisplayReg)
					updateVideoParams();
				break;
			case 0xFFD8:
				cpuClockDivisor = 32;
				break;
			case 0xFFD9:
				cpuClockDivisor = 16;
				break;
			case 0xFFDE:
			case 0xFFDF:
				gimeAllRamModeEnabled = (address & 0x0001);
				break;
			}
		}
	}
	// If here, the Logical address is less than 0xFF00
	else
		writePhysicalByte(address, byte);
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
