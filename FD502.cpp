#include "GimeBus.h"
#include "FD502.h"

FD502::FD502()
{
	isConnected = true;
	stepWaitCounter = 0;
	fdcPendingCommand = 0;
	fdcHeadLoaded = false;
	verifyAfterCommand = false;
	for (int i = 0; i < 3; i++)
	{
		fdcDrive[i].isDiskInserted = false;
		fdcDrive[i].isDriveAttached = false;
		fdcDrive[i].diskImageFile = nullptr;
	}
}

void FD502::fdcAttachNewDrive(uint8_t driveNum, uint8_t totalTracks, bool isDoubleSided, bool isDoubleDensity)
{
	if (!fdcDrive[driveNum].isDriveAttached)
	{
		fdcDrive[driveNum].totalCylinders = totalTracks;
		fdcDrive[driveNum].isDoubleSided = isDoubleSided;
		fdcDrive[driveNum].isDoubleDensity = isDoubleDensity;
		fdcDrive[driveNum].isDriveAttached = true;
	}
}

void FD502::fdcDetachDrive(uint8_t driveNum)
{
	if (fdcDrive[driveNum].isDriveAttached)
		fdcDrive[driveNum].isDriveAttached = false;
}

uint8_t FD502::fdcInsertDisk(uint8_t driveNum, std::string diskImageFilePath)
{
	if (!fdcDrive[driveNum].isDriveAttached)
		return FD502_ERROR_DRIVE_NOT_ATTACHED;

	fdcDrive[driveNum].diskImageFile = fopen(diskImageFilePath.c_str(), "rb+");
	if (fdcDrive[driveNum].diskImageFile == NULL)
	//if (fopen_s(&fdcDrive[driveNum].diskImageFile, diskImageFilePath.c_str(), "rb+") != 0)
		return FD502_ERROR_OPENING_DISK_IMAGE;

	// At least for now, guess at the disk's geometry based on filesize
	fseek(fdcDrive[driveNum].diskImageFile, 0, SEEK_END);
	long diskImageFilesize = ftell(fdcDrive[driveNum].diskImageFile);
	//fseek(fdcDrive[driveNum].diskImageFile, 0, SEEK_SET);				// Return file pointer to beginning of file again
	fdcDrive[driveNum].diskImageGeometry = nullptr;
	for (int i = 0; i < (sizeof(diskImageGeometry) / sizeof(diskImageGeometry[0])); i++)
		if (diskImageFilesize == diskImageGeometry[i].totalFilesize)
		{
			fdcDrive[driveNum].diskImageGeometry = &diskImageGeometry[i];
			fdcDrive[driveNum].isDiskInserted = true;
			return FD502_OPERATION_COMPLETE;
		}
	return FD502_ERROR_UNSUPPORTED_DISK_IMAGE;
}

uint32_t FD502::getDiskImageOffset(uint8_t driveNum, uint8_t trackNum, uint8_t sectorNum, uint8_t sideNum)
{
	// IMPORTANT: The math below assumes all entry parameters (track number, sector number, side number) are zero-base numbers
	diskImageStruct* diskGeometry = fdcDrive[driveNum].diskImageGeometry;	// This pointer is just so we can use a shortened code reference to make things easier to read
	return (diskGeometry->bytesPerTrack * (trackNum * diskGeometry->totalSides)) + (diskGeometry->bytesPerTrack * sideNum) + (diskGeometry->bytesPerSector * sectorNum);
}

void FD502::fdcRegisterWrite(uint16_t regAddress, uint8_t paramByte)
{
	if (regAddress == 0xFF40)
	{
		fdcHaltFlag = paramByte & 0x80;
		if (fdcHaltFlag)
		{ 
			if (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST)
				gimeBus->cpu.cpuHardwareHalt = false;
			else
				gimeBus->cpu.cpuHaltAsserted = true;
		}
		else
			gimeBus->cpu.cpuHardwareHalt = false;
		fdcDoubleDensity = paramByte & 0x20;
		fdcWritePrecompensation = paramByte & 0x10;
		if (!fdcMotorOn && (paramByte & 0x08))
		{
			printf("Floppy motor on.\n");
			gimeBus->statusBarText = "FDC: Motor on";
		}
		else if (fdcMotorOn && !(paramByte & 0x08))
		{
			printf("Floppy motor off.\n");
			gimeBus->statusBarText = "FDC: Motor off";
		}
		fdcMotorOn = paramByte & 0x08;
		fdcSideSelect = (paramByte & 0b01000000) >> 6;
		if (paramByte & 0b00000100)
			fdcDriveSelect = 2;
		else if (paramByte & 0b00000010)
			fdcDriveSelect = 1;
		else if (paramByte & 0b00000001)
			fdcDriveSelect = 0;
		else
			fdcDriveSelect = 0xFF;			// 0xFF indicates no drive select line has been set
	}
	else if (regAddress == 0xFF48)
	{
		switch (paramByte & 0b11100000)
		{
		case FDC_CMD_PREFIX_RESTORE_SEEK:
			if (paramByte & 0b00010000)
				fdcSeekCommand(paramByte);
			else
				fdcRestoreCommand(paramByte);
			break;
		case FDC_CMD_PREFIX_STEP:
			fdcStep(paramByte);
			break;
		case FDC_CMD_PREFIX_STEP_IN:
			fdcStepIn(paramByte);
			break;
		case FDC_CMD_PREFIX_STEP_OUT:
			fdcStepOut(paramByte);
			break;
		case FDC_CMD_PREFIX_READ_SECTOR:
			fdcReadSector();
			break;
		case FDC_CMD_PREFIX_WRITE_SECTOR:
			fdcWriteSector();
			break;
		case FDC_CMD_PREFIX_READ_ADDR_FORCE_INTERRUPT:
			fdcForceInterrupt(paramByte & 0x0F);
			break;
		case FDC_CMD_PREFIX_READ_WRITE_TRACK:
			break;
		}
	}
	else if (regAddress == 0xFF49)
		fdcTrackReg = paramByte;
	else if (regAddress == 0xFF4A)
		fdcSectorReg = paramByte;
	else if (regAddress == 0xFF4B)
	{
		fdcStatusReg &= ~FDC_STATUS_II_III_DATA_REQUEST;		// Clear the DRQ flag from status register
		if (fdcHaltFlag)
			gimeBus->cpu.cpuHaltAsserted = true;
		fdcDataReg = paramByte;
	}
}

uint8_t FD502::fdcRegisterRead(uint16_t regAddress)
{
	switch (regAddress)
	{
	case 0xFF48:
		return fdcStatusReg;
	case 0xFF49:
		return fdcTrackReg;
	case 0xFF4A:
		return fdcSectorReg;
	case 0xFF4B:
		fdcStatusReg &= ~FDC_STATUS_II_III_DATA_REQUEST;		// Clear the DRQ flag from status register
		if (fdcHaltFlag)
			gimeBus->cpu.cpuHaltAsserted = true;
		return fdcDataReg;
	}
}

uint8_t FD502::fdcHandleNextEvent()
{
	switch (fdcPendingCommand)
	{
	case FDC_OP_NONE:
		return FD502_OPERATION_COMPLETE;			// No pending floppy operation. Do nothing and return
	case FDC_OP_RESTORE:
		// Restore command in progress
		stepWaitCounter--;
		if (stepWaitCounter == 0)
		{
			if (physicalTrackPosition == 0)
			{
				fdcTrackReg = 0;
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~FDC_STATUS_I_BUSY;
				fdcStatusReg &= ~FDC_STATUS_I_SEEK_ERROR;		// Clear SEEK ERROR flag in status since we successfully made it to Track 0
				return FD502_OPERATION_COMPLETE;
			}
			else
			{
				physicalTrackPosition--;
				lastStepDirection = FDC_LAST_STEPPED_OUT;
				stepWaitCounter = curStepRate;
			}	
		}
		return FDC_OP_RESTORE;
		break;
	case FDC_OP_SEEK:
		// Seek command in progress
		stepWaitCounter--;
		if (stepWaitCounter == 0)
		{
			if (physicalTrackPosition == destinationTrack)
			{
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~FDC_STATUS_I_BUSY;				// Clear BUSY flag in status
				// If the "Verify" flag was set in the command byte, verify the track we ended up on is valid for the mounted disk image. If it's not, set the SEEK ERROR flag.
				if (verifyAfterCommand && (!fdcDrive[fdcDriveSelect].isDiskInserted || (physicalTrackPosition > fdcDrive[fdcDriveSelect].diskImageGeometry->tracksPerSide)))
					fdcStatusReg |= FDC_STATUS_I_SEEK_ERROR;
				else
					fdcStatusReg &= ~FDC_STATUS_I_SEEK_ERROR;		// Clear SEEK ERROR flag in status since we successfully stepped to requested track
				return FD502_OPERATION_COMPLETE;
			}
			else
			{
				if (physicalTrackPosition > destinationTrack)
				{
					if (physicalTrackPosition > 0)
					{
						physicalTrackPosition--;
						fdcTrackReg--;
					}
					lastStepDirection = FDC_LAST_STEPPED_OUT;
				}
				else
				{
					if (physicalTrackPosition < fdcDrive[fdcDriveSelect].totalCylinders)
					{
						physicalTrackPosition++;
						fdcTrackReg++;
					}
					lastStepDirection = FDC_LAST_STEPPED_IN;
				}
				stepWaitCounter = curStepRate;
			}
		}
		return FDC_OP_SEEK;
		break;
	case FDC_OP_STEP:
		// Step command in progress
		stepWaitCounter--;
		if (stepWaitCounter == 0)
		{
			if (lastStepDirection == FDC_LAST_STEPPED_OUT)
			{
				if (physicalTrackPosition > 0)
					physicalTrackPosition--;
				if (updateTrackRegFlag && (fdcTrackReg > 0))
					fdcTrackReg--;
			}
			else if (lastStepDirection == FDC_LAST_STEPPED_IN)
			{
				if (physicalTrackPosition < fdcDrive[fdcDriveSelect].totalCylinders)
					physicalTrackPosition++;
				if (updateTrackRegFlag && (fdcTrackReg < fdcDrive[fdcDriveSelect].totalCylinders))
					fdcTrackReg++;
			}
			fdcPendingCommand = FDC_OP_NONE;
			fdcStatusReg &= ~FDC_STATUS_I_BUSY;
			return FD502_OPERATION_COMPLETE;
		}
		return FDC_OP_STEP;
		break;
	case FDC_OP_STEP_IN:
		// Step-in command in progress
		stepWaitCounter--;
		if (stepWaitCounter == 0)
		{
			if (physicalTrackPosition < 76)
			{
				physicalTrackPosition++;
				lastStepDirection = FDC_LAST_STEPPED_IN;
			}
			if (updateTrackRegFlag && (fdcTrackReg < fdcDrive[fdcDriveSelect].totalCylinders))
				fdcTrackReg++;
			fdcPendingCommand = FDC_OP_NONE;
			fdcStatusReg &= ~FDC_STATUS_I_BUSY;
			return FD502_OPERATION_COMPLETE;
		}
		return FDC_OP_STEP_IN;
		break;
	case FDC_OP_STEP_OUT:
		// Step-out command in progress
		stepWaitCounter--;
		if (stepWaitCounter == 0)
		{
			if (physicalTrackPosition > 0)
			{
				physicalTrackPosition--;
				lastStepDirection = FDC_LAST_STEPPED_OUT;
			}
			if (updateTrackRegFlag && (fdcTrackReg > 0))
				fdcTrackReg--;
			fdcPendingCommand = FDC_OP_NONE;
			fdcStatusReg &= ~FDC_STATUS_I_BUSY;
			return FD502_OPERATION_COMPLETE;
		}
		return FDC_OP_STEP_OUT;
		break;
	case FDC_OP_READ_SECTOR:
		// Read Sector command in progress
		if ((fdcDriveSelect == 0xFF) || !fdcDrive[fdcDriveSelect].isDriveAttached)
			return FD502_ERROR_DRIVE_NOT_ATTACHED;
		else if (!fdcDrive[fdcDriveSelect].isDiskInserted)
			return FD502_ERROR_NO_DISK_INSERTED;
		else 
		{
			if (fdcStatusReg & FDC_STATUS_II_III_RECORD_NOT_FOUND)
			{
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				return FD502_OPERATION_COMPLETE;
			}
			else if (bufferPosition < fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector)
			{
				if (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST)
					fdcStatusReg |= FDC_STATUS_II_III_LOST_DATA;

				if (bufferPosition == 0)
				{
					// If bufferPosition is 0, then we need to retreive a new sector from the disk image before returning first byte of new sector
					printf("FDC: Reading Track %02u Sector %02u Side %01u - Checksum = ", fdcTrackReg, fdcSectorReg, fdcSideSelect);
					if (fdcDrive[fdcDriveSelect].diskImageGeometry->totalSides > 1)
						snprintf(textBuffer, sizeof(textBuffer), "FDC: Reading Track %02u Sector %02u Side %01u", fdcTrackReg, fdcSectorReg, fdcSideSelect);
					else
						snprintf(textBuffer, sizeof(textBuffer), "FDC: Reading Track %02u Sector %02u", fdcTrackReg, fdcSectorReg);
					gimeBus->statusBarText = textBuffer;
					uint32_t diskImageOffset = getDiskImageOffset(fdcDriveSelect, fdcTrackReg, fdcSectorReg - 1, fdcSideSelect);
					fseek(fdcDrive[fdcDriveSelect].diskImageFile, diskImageOffset, SEEK_SET);
					fread(&sectorBuffer, 1, fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector, fdcDrive[fdcDriveSelect].diskImageFile);
				}
				fdcDataReg = sectorBuffer[bufferPosition];

				// FOR DEBUG ONLY
				checksumByte += fdcDataReg;

				fdcStatusReg |= FDC_STATUS_II_III_DATA_REQUEST;
				bufferPosition++;
				if (fdcHaltFlag)
					gimeBus->cpu.cpuHardwareHalt = false;
			}
			else
			{
				// If here, we have finished loading in every byte in the sector. Sector Read command has completed.
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				fdcHaltFlag = false;
				gimeBus->cpu.cpuHardwareHalt = false;

				// FOR DEBUG ONLY
				checksumByte = (~checksumByte) + 1;
				printf("%02X\n", checksumByte);

				return FD502_OPERATION_COMPLETE;
			}
		}
		return FDC_OP_READ_SECTOR;
		break;
	case FDC_OP_WRITE_SECTOR:
		// Write Sector command in progress
		if ((fdcDriveSelect == 0xFF) || !fdcDrive[fdcDriveSelect].isDriveAttached)
			return FD502_ERROR_DRIVE_NOT_ATTACHED;
		else if (!fdcDrive[fdcDriveSelect].isDiskInserted)
			return FD502_ERROR_NO_DISK_INSERTED;
		else
		{
			if (fdcStatusReg & FDC_STATUS_II_III_RECORD_NOT_FOUND)
			{
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				return FD502_OPERATION_COMPLETE;
			}
			else if (bufferPosition < fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector)
			{
				/*
				if ((bufferPosition == 0) && (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST))
				{
					// The DRQ was not serviced by the CPU in time. For sector writes, a Lost Data event automatically terminates the command when it happens on the first byte.
					fdcStatusReg |= FDC_STATUS_II_III_LOST_DATA;
					fdcStatusReg &= ~FDC_STATUS_II_III_BUSY;
					fdcPendingCommand = FDC_OP_NONE;
					printf("FDC: Lost Data occured on first byte of Write Sector. Command terminated.\n");
					return FD502_OPERATION_COMPLETE;
				}
				*/
				if (bufferPosition == 0)
				{
					// If bufferPosition is 0, then we need to start building a sector buffer cache that when full, will actually be written to the Host's image file
					printf("FDC: Writing Track %02u Sector %02u Side %01u\n", fdcTrackReg, fdcSectorReg, fdcSideSelect);
					if (fdcDrive[fdcDriveSelect].diskImageGeometry->totalSides > 1)
						snprintf(textBuffer, sizeof(textBuffer), "FDC: Writing Track %02u Sector %02u Side %01u", fdcTrackReg, fdcSectorReg, fdcSideSelect);
					else
						snprintf(textBuffer, sizeof(textBuffer), "FDC: Writing Track %02u Sector %02u", fdcTrackReg, fdcSectorReg);
					gimeBus->statusBarText = textBuffer;
					//uint32_t diskImageOffset = getDiskImageOffset(fdcDriveSelect, fdcTrackReg, fdcSectorReg - 1, fdcSideSelect);
					//fseek(fdcDrive[fdcDriveSelect].diskImageFile, diskImageOffset, SEEK_SET);
					//fread(&sectorBuffer, 1, fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector, fdcDrive[fdcDriveSelect].diskImageFile);
					sectorBuffer[bufferPosition] = fdcDataReg;
				}
				else if (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST)
				{
					fdcStatusReg |= FDC_STATUS_II_III_LOST_DATA;
					printf("FDC: Lost Data during continious writes\n");
					gimeBus->cpu.debuggerStepEnabled = true;
					sectorBuffer[bufferPosition] = 0x00;	// On sector writes, when a Lost Data event occurs AFTER the first byte, a zero is written to disk instead of valid data
				}
				else
					sectorBuffer[bufferPosition] = fdcDataReg;
				bufferPosition++;
				fdcStatusReg |= FDC_STATUS_II_III_DATA_REQUEST;
				if (fdcHaltFlag)
					gimeBus->cpu.cpuHardwareHalt = false;
			}
			else
			{
				// If here, we have finished building our complete sector buffer. Write it to our Host device's disk image file and our Write Sector command is now complete.
				uint32_t diskImageOffset = getDiskImageOffset(fdcDriveSelect, fdcTrackReg, fdcSectorReg - 1, fdcSideSelect);
				fseek(fdcDrive[fdcDriveSelect].diskImageFile, diskImageOffset, SEEK_SET);
				fwrite(&sectorBuffer, 1, fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector, fdcDrive[fdcDriveSelect].diskImageFile);
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				fdcHaltFlag = false;
				gimeBus->cpu.cpuHardwareHalt = false;

				return FD502_OPERATION_COMPLETE;
			}
		}
		break;
	}
}

void FD502::fdcRestoreCommand(uint8_t commandByte)
{
	printf("FDC: Restore to Track 0\n");
	gimeBus->statusBarText = "FDC: Restoring head to Track 0";
	physicalTrackPosition = fdcTrackReg;
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	fdcPendingCommand = FDC_OP_RESTORE;
	fdcStatusReg |= FDC_STATUS_I_BUSY;
	stepWaitCounter = curStepRate;
}

void FD502::fdcSeekCommand(uint8_t commandByte)
{
	printf("FDC: Seeking to Track %02u\n", fdcDataReg);
	snprintf(textBuffer, sizeof(textBuffer), "FDC: Seeking to Track %02u", fdcDataReg);
	gimeBus->statusBarText = textBuffer;
	destinationTrack = fdcDataReg;
	physicalTrackPosition = fdcTrackReg;
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	fdcPendingCommand = FDC_OP_SEEK;
	fdcStatusReg |= FDC_STATUS_I_BUSY;
	stepWaitCounter = curStepRate;
}

void FD502::fdcStep(uint8_t commandByte)
{
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	updateTrackRegFlag = commandByte & 0b00010000;
	fdcPendingCommand = FDC_OP_STEP;
	fdcStatusReg |= FDC_STATUS_I_BUSY;
	stepWaitCounter = curStepRate;
}

void FD502::fdcStepIn(uint8_t commandByte)
{
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	updateTrackRegFlag = commandByte & 0b00010000;
	fdcPendingCommand = FDC_OP_STEP_IN;
	fdcStatusReg |= FDC_STATUS_I_BUSY;
	stepWaitCounter = curStepRate;
}

void FD502::fdcStepOut(uint8_t commandByte)
{
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	updateTrackRegFlag = commandByte & 0b00010000;
	fdcPendingCommand = FDC_OP_STEP_OUT;
	fdcStatusReg |= FDC_STATUS_I_BUSY;
	stepWaitCounter = curStepRate;
}

void FD502::fdcForceInterrupt(uint8_t interruptType)
{
	printf("FDC: Force Interrupt\n");
	fdcPendingCommand = FDC_OP_NONE;
	fdcStatusReg &= ~FDC_STATUS_II_III_BUSY;
	if (interruptType & 0b00001000)
		gimeBus->cpu.assertedInterrupts[INT_NMI] |= INT_ASSERT_MASK_NMI;
}

uint8_t FD502::fdcReadSector()
{
	// First, make sure the requested sector and track are not out of range for our mounted disk image
	if ((fdcSectorReg <= fdcDrive[fdcDriveSelect].diskImageGeometry->sectorsPerTrack) && (fdcTrackReg < fdcDrive[fdcDriveSelect].diskImageGeometry->tracksPerSide))
	{
		fdcStatusReg |= FDC_STATUS_II_III_BUSY;		
		fdcStatusReg &= ~FDC_STATUS_II_III_RECORD_NOT_FOUND;
		bufferPosition = 0;
		fdcPendingCommand = FDC_OP_READ_SECTOR;
		//stepWaitCounter = curStepRate;

		// FOR DEBUG ONLY
		checksumByte = 0;
	}
	else
	{
		fdcStatusReg |= FDC_STATUS_II_III_RECORD_NOT_FOUND;
		fdcPendingCommand = FDC_OP_READ_SECTOR;
		printf("FDC: Record Not Found\n");
	}
	fdcHeadLoaded = false;
	return 0;
}

uint8_t FD502::fdcWriteSector()
{
	// First, make sure the requested sector and track are not out of range for our mounted disk image
	if ((fdcSectorReg <= fdcDrive[fdcDriveSelect].diskImageGeometry->sectorsPerTrack) && (fdcTrackReg < fdcDrive[fdcDriveSelect].diskImageGeometry->tracksPerSide))
	{
		fdcStatusReg |= FDC_STATUS_II_III_BUSY;
		fdcStatusReg &= ~FDC_STATUS_II_III_RECORD_NOT_FOUND;
		bufferPosition = 0;
		fdcPendingCommand = FDC_OP_WRITE_SECTOR;
	}
	else
	{
		fdcStatusReg |= FDC_STATUS_II_III_RECORD_NOT_FOUND;
		fdcPendingCommand = FDC_OP_WRITE_SECTOR;
		printf("FDC: Record Not Found\n");
	}
	fdcHeadLoaded = false;

	// FOR DEBUG ONLY
	//gimeBus->cpu.debuggerStepEnabled = true;

	return 0;
}