#include "GimeBus.h"
#include "FD502.h"

FD502::FD502()
{
	isConnected = true;
	stepWaitCounter = 0;
	fdcPendingCommand = 0;
	fdcHeadLoaded = false;
	verifyAfterCommand = false;
	fdcIndexPulse = false;
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
		fdcDrive[driveNum].isDiskInserted = false;
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

	// Check if a disk image is already mounted in this drive and if so, close it before attempting to mount new one
	if (fdcDrive[driveNum].isDiskInserted)
		fdcEjectDisk(driveNum);

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
			fdcDrive[driveNum].imgFilePathname = diskImageFilePath;
			return FD502_OPERATION_COMPLETE;
		}
	return FD502_ERROR_UNSUPPORTED_DISK_IMAGE;
}

uint8_t FD502::fdcEjectDisk(uint8_t driveNum)
{
	if (!fdcDrive[driveNum].isDriveAttached)
		return FD502_ERROR_DRIVE_NOT_ATTACHED;
	if (!fdcDrive[driveNum].isDiskInserted)
		return FD502_ERROR_NO_DISK_INSERTED;

	fclose(fdcDrive[driveNum].diskImageFile);
	fdcDrive[driveNum].imgFilePathname.clear();
	fdcDrive[driveNum].diskImageGeometry = nullptr;
	fdcDrive[driveNum].isDiskInserted = false;
	return FD502_OPERATION_COMPLETE;
}

bool FD502::getDiskImageOffset(uint8_t driveNum, uint8_t trackNum, uint8_t sectorNum, uint8_t sideNum)
{
	diskImageStruct* diskGeometry = fdcDrive[driveNum].diskImageGeometry;	// This pointer is just so we can use a shortened code reference to make things easier to read

	// Attempt some basic validation of the parameters
	if ((driveNum > 2) || (trackNum > 80) || (sideNum > 1))
		return false;
	if ((diskGeometry->bytesPerSector == 256) && (sectorNum > 0x1A))
		return false;
	else if ((diskGeometry->bytesPerSector == 512) && (sectorNum > 0x0F))
		return false;
	else if ((diskGeometry->bytesPerSector == 1024) && (sectorNum > 0x08))
		return false;

	// IMPORTANT: The math below assumes all entry parameters (track number, sector number, side number) are zero-base numbers
	diskImageOffset = (diskGeometry->bytesPerTrack * (trackNum * diskGeometry->totalSides)) + (diskGeometry->bytesPerTrack * sideNum) + (diskGeometry->bytesPerSector * sectorNum);
	return true;
}

void FD502::fdcRegisterWrite(uint16_t regAddress, uint8_t paramByte)
{
	if (regAddress == 0xFF40)
	{
		if (!fdcHaltFlag && (paramByte & 0x80))
		{
			fdcHaltFlag = paramByte & 0x80;
			if (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST)
			{
				gimeBus->cpu.cpuHardwareHalt = false;
				gimeBus->cpu.cpuHaltAsserted = false;
			}
			else
				gimeBus->cpu.cpuHaltAsserted = true;
		}
		else if (fdcHaltFlag && !(paramByte & 0x80))
		{
			fdcHaltFlag = paramByte & 0x80;
			gimeBus->cpu.cpuHardwareHalt = false;
			gimeBus->cpu.cpuHaltAsserted = false;
		}
		fdcDoubleDensity = paramByte & 0x20;
		fdcWritePrecompensation = paramByte & 0x10;
		if (!fdcMotorOn && (paramByte & 0x08))
		{
			gimeBus->floppyIndexHoleCounter = 0;
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
			if (paramByte & 0b00010000)
				fdcWriteTrack();
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
				fdcStatusReg &= ~(FDC_STATUS_I_BUSY | FDC_STATUS_I_SEEK_ERROR);		// Clear Seek Error flag and Busy flag since we successfully made it Track 0
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
				if (fdcHaltFlag)
				{
					gimeBus->cpu.cpuHardwareHalt = false;
					gimeBus->cpu.cpuHaltAsserted = false;
				}
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
					if (!getDiskImageOffset(fdcDriveSelect, fdcTrackReg, fdcSectorReg - 1, fdcSideSelect))
					{
						fdcStatusReg |= FDC_STATUS_II_III_RECORD_NOT_FOUND;
						return FDC_OP_READ_SECTOR;
					}
					fseek(fdcDrive[fdcDriveSelect].diskImageFile, diskImageOffset, SEEK_SET);
					fread(&sectorBuffer, 1, fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector, fdcDrive[fdcDriveSelect].diskImageFile);
				}
				fdcDataReg = sectorBuffer[bufferPosition];

				// FOR DEBUG ONLY
				checksumByte += fdcDataReg;

				fdcStatusReg |= FDC_STATUS_II_III_DATA_REQUEST;
				bufferPosition++;
				if (fdcHaltFlag)
				{
					gimeBus->cpu.cpuHardwareHalt = false;
					gimeBus->cpu.cpuHaltAsserted = false;
				}
			}
			else
			{
				// If here, we have finished loading in every byte in the sector. Sector Read command has completed.
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				fdcHaltFlag = false;
				gimeBus->cpu.cpuHardwareHalt = false;
				gimeBus->cpu.cpuHaltAsserted = false;

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
				if (fdcHaltFlag)
				{
					gimeBus->cpu.cpuHardwareHalt = false;
					gimeBus->cpu.cpuHaltAsserted = false;
				}
				return FD502_OPERATION_COMPLETE;
			}
			else if (bufferPosition < fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector)
			{
				// In double-density mode, the FDC waits for 22 "byte" times before it checks if DRQ was serviced and is ready to write data
				if (fdcByteDelayCounter < 22)
				{
					fdcByteDelayCounter++;
					return FDC_OP_WRITE_SECTOR;
				}

				if (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST)
					fdcStatusReg |= FDC_STATUS_II_III_LOST_DATA;

				if (bufferPosition == 0)
				{
					// If bufferPosition is 0, then we need to start building a sector buffer cache that when full, will actually be written to the Host's image file
					printf("FDC: Writing Track %02u Sector %02u Side %01u\n", fdcTrackReg, fdcSectorReg, fdcSideSelect);
					if (fdcDrive[fdcDriveSelect].diskImageGeometry->totalSides > 1)
						snprintf(textBuffer, sizeof(textBuffer), "FDC: Writing Track %02u Sector %02u Side %01u", fdcTrackReg, fdcSectorReg, fdcSideSelect);
					else
						snprintf(textBuffer, sizeof(textBuffer), "FDC: Writing Track %02u Sector %02u", fdcTrackReg, fdcSectorReg);
					gimeBus->statusBarText = textBuffer;

					if (fdcStatusReg & FDC_STATUS_II_III_LOST_DATA)
					{
						// If Lost Data event occurs on first byte of Write Sector command, the command is immediately terminated
						printf("FDC: Lost Data writing first byte!\n");
						fdcPendingCommand = FDC_OP_NONE;
						fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
						fdcHaltFlag = false;
						gimeBus->cpu.cpuHardwareHalt = false;
						gimeBus->cpu.cpuHaltAsserted = false;
						gimeBus->cpu.debuggerStepEnabled = true;
						return FDC_OP_NONE;
					}
					else
						sectorBuffer[bufferPosition] = fdcDataReg;
				}
				else if (fdcStatusReg & FDC_STATUS_II_III_LOST_DATA)
				{
					printf("FDC: Lost Data in the middle of writing bytes!\n");
					gimeBus->cpu.debuggerStepEnabled = true;
					sectorBuffer[bufferPosition] = 0x00;	// On sector writes, when a Lost Data event occurs AFTER the first byte, a zero is written to disk instead of valid data
				}
				else
					sectorBuffer[bufferPosition] = fdcDataReg;
				bufferPosition++;
				fdcStatusReg |= FDC_STATUS_II_III_DATA_REQUEST;
				if (fdcHaltFlag)
				{
					gimeBus->cpu.cpuHardwareHalt = false;
					gimeBus->cpu.cpuHaltAsserted = false;
				}
			}
			else
			{
				// If here, we have finished building our complete sector buffer. Write it to our Host device's disk image file and our Write Sector command is now complete.
				if (!getDiskImageOffset(fdcDriveSelect, fdcTrackReg, fdcSectorReg - 1, fdcSideSelect))
				{
					fdcStatusReg |= FDC_STATUS_II_III_RECORD_NOT_FOUND;
					return FDC_OP_WRITE_SECTOR;
				}
				fseek(fdcDrive[fdcDriveSelect].diskImageFile, diskImageOffset, SEEK_SET);
				fwrite(&sectorBuffer, 1, fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector, fdcDrive[fdcDriveSelect].diskImageFile);
				fflush(fdcDrive[fdcDriveSelect].diskImageFile);
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				fdcHaltFlag = false;
				gimeBus->cpu.cpuHardwareHalt = false;
				gimeBus->cpu.cpuHaltAsserted = false;

				return FD502_OPERATION_COMPLETE;
			}
			return FDC_OP_WRITE_SECTOR;
		}
		break;
	case FDC_OP_WRITE_TRACK:
		// Write Track (aka FORMATTING) in Progress
		if ((fdcDriveSelect == 0xFF) || !fdcDrive[fdcDriveSelect].isDriveAttached)
			return FD502_ERROR_DRIVE_NOT_ATTACHED;
		else if (!fdcDrive[fdcDriveSelect].isDiskInserted)
			return FD502_ERROR_NO_DISK_INSERTED;
		else
		{
			// Write Track waits until it encounters leading edge of Index Pulse before starting to write bytes, so until then, just return
			if (!formatWritingStarted)
			{
				if (fdcIndexPulse)
					formatWritingStarted = true;
				else
					return FDC_OP_WRITE_TRACK;
			}

			// If this is the very beginning of command and DRQ remains set for 3 consecutive "byte times", then command is terminated, lost data flag set, and drive busy flag cleared
			// If we are in the middle of Write Track command though and DRQ is set, substitute a 0x00 for missing byte and set Lost Data flag, but continue on with the command
			if (fdcStatusReg & FDC_STATUS_II_III_DATA_REQUEST)
			{
				if (formatTotalBytesCounter == 0)
				{
					fdcByteDelayCounter++;
					if (fdcByteDelayCounter >= 3)
					{
						fdcStatusReg &= ~FDC_STATUS_II_III_BUSY;
						fdcStatusReg |= FDC_STATUS_II_III_LOST_DATA;
						printf("FDC: Lost Data during WRITE TRACK\n");
						fdcHaltFlag = false;
						gimeBus->cpu.cpuHardwareHalt = false;
						gimeBus->cpu.cpuHaltAsserted = false;
						fdcPendingCommand = FDC_OP_NONE;
						return FD502_OPERATION_COMPLETE;
					}
					else
						return FDC_OP_WRITE_TRACK;
				}
				else
				{
					fdcStatusReg |= FDC_STATUS_II_III_LOST_DATA;
					printf("FDC: Lost Data during WRITE TRACK\n");
					gimeBus->cpu.debuggerStepEnabled = true;
					formatByte = 0x00;
				}
			}
			else
				formatByte = fdcDataReg;

			// For now, we will filter out the GAP, ID, Checksum bytes etc
			switch (curFormatSegment)
			{
			case FDC_FORMAT_SEGMENT_GAP5:
				// First check for optional Index AM sync bytes
				if (formatByte == 0xF6)				// This is the first of 3 Index AM sync bytes (all the same value)
					curFormatSegment = FDC_FORMAT_SEGMENT_INDEX_AM_SYNC;
				// If Index AM section is omitted, check for ID AM Sync bytes
				else if (formatByte == 0xF5)		// This is the first of 3 ID AM sync bytes (all the same value)
					curFormatSegment = FDC_FORMAT_SEGMENT_ID_AM_SYNC;
				// I could be counting gap bytes, but there may be some variation in those counts so i'll just ignore everything else
				break;
			case FDC_FORMAT_SEGMENT_INDEX_AM_SYNC:
				if (formatByte == 0xFC)
					curFormatSegment = FDC_FORMAT_SEGMENT_GAP1;		// We've received the Index AM byte, next section is Gap 1
				else if (formatByte != 0xF6)
					printf("FDC: Invalid byte in Index AM Sync! (%02X)\n", formatByte);
				break;
			case FDC_FORMAT_SEGMENT_GAP1:
				if (formatByte == 0xF5)				// This is the first of 3 ID AM sync bytes (all the same value)
					curFormatSegment = FDC_FORMAT_SEGMENT_ID_AM_SYNC;
				// I could be counting gap bytes, but there may be some variation in those counts so i'll just ignore everything else
				break;
			case FDC_FORMAT_SEGMENT_ID_AM_SYNC:
				if (formatByte == 0xFE)
					curFormatSegment = FDC_FORMAT_SEGMENT_SECTOR_ID;	// We've received the ID AM byte. The next section is all the Sector ID bytes
				else if (formatByte != 0xF5)
					printf("FDC: Invalid byte in ID AM Sync! (%02X)\n", formatByte);
				break;
			case FDC_FORMAT_SEGMENT_SECTOR_ID:
				if ((formatByte > 0x4C) && (formatSectorInfoIndex < FDC_SECTOR_INFO_LENGTH))
					printf("FDC: Invalid value in 4-byte Sector ID! (%02X)\n", formatByte);
				else if (formatSectorInfoIndex < 4)
				{
					formatSectorInfo[formatSectorInfoIndex] = formatByte;
					formatSectorInfoIndex++;
				}
				else if (formatByte == 0xF7)
					curFormatSegment = FDC_FORMAT_SEGMENT_GAP2;
				else
					printf("FDC: 4-byte Sector ID did not end with CRC byte! Expected F7, received %02X\n", formatByte);
				break;
			case FDC_FORMAT_SEGMENT_GAP2:
				if (formatByte == 0xF5)		// This is the first of 3 "Data AM" sync bytes (all the same value)
					curFormatSegment = FDC_FORMAT_SEGMENT_DATA_AM_SYNC;
				// I could be counting gap bytes, but there may be some variation in those counts so i'll just ignore everything else
				break;
			case FDC_FORMAT_SEGMENT_DATA_AM_SYNC:
				if (formatByte == 0xFB)
					curFormatSegment = FDC_FORMAT_SEGMENT_DATA_FIELD;	// We've received the Data AM byte. The next section is the actual sector data
				else if (formatByte != 0xF5)
					printf("FDC: Invalid byte in Data AM Sync! (%02X)\n", formatByte);
				break;
			case FDC_FORMAT_SEGMENT_DATA_FIELD:
				if (formatSectorByteCount < sectorLengthTable[formatSectorInfo[FDC_SECTOR_INFO_LENGTH]])
				{
					sectorBuffer[formatSectorByteCount] = formatByte;
					formatSectorByteCount++;
				}
				else if (formatSectorByteCount == sectorLengthTable[formatSectorInfo[FDC_SECTOR_INFO_LENGTH]])
				{
					if (formatByte == 0xF7)
					{
						// At this point, we would have a complete sector written to the track. Output it to our disk image.
						if (getDiskImageOffset(fdcDriveSelect, fdcTrackReg, formatSectorInfo[FDC_SECTOR_INFO_SECTOR] - 1, fdcSideSelect))
						{
							fseek(fdcDrive[fdcDriveSelect].diskImageFile, diskImageOffset, SEEK_SET);
							fwrite(&sectorBuffer, 1, fdcDrive[fdcDriveSelect].diskImageGeometry->bytesPerSector, fdcDrive[fdcDriveSelect].diskImageFile);
							fflush(fdcDrive[fdcDriveSelect].diskImageFile);
							//printf("Physically wrote a sector to Host PC filesystem\n");
						}
						else
							printf("FDC: Could not calculate Disk Image Offset due to invalid Sector ID parameters!\n");

						curFormatSegment = FDC_FORMAT_SEGMENT_GAP3;
					}
					else
						printf("FDC: Invalid CRC byte after the Data Field. Expected F7, received %02X\n", formatByte);
				}
				break;
			case FDC_FORMAT_SEGMENT_GAP3:
				// If we read a value of $00 while in GAP3, it means we have wrapped into the next sector's starting gap, so we loop back to GAP1 handler
				// Otherwise, the gap bytes transition into the final pre-index gap until the Write Track command completes
				if (formatByte == 0x00)
				{
					formatSectorInfoIndex = 0;
					formatSectorByteCount = 0;
					curFormatSegment = FDC_FORMAT_SEGMENT_GAP1;
				}
				break;
			}

			formatTotalBytesCounter++;
			//if (formatTotalBytesCounter == 6250)
			if (fdcIndexPulse && (curFormatSegment == FDC_FORMAT_SEGMENT_GAP3))
			{
				fdcPendingCommand = FDC_OP_NONE;
				fdcStatusReg &= ~(FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);		// clear both BUSY and DRQ flags
				fdcHaltFlag = false;
				gimeBus->cpu.cpuHardwareHalt = false;
				gimeBus->cpu.cpuHaltAsserted = false;
				return FD502_OPERATION_COMPLETE;
			}
			else
			{
				fdcStatusReg |= FDC_STATUS_II_III_DATA_REQUEST;
				if (fdcHaltFlag)
				{
					gimeBus->cpu.cpuHardwareHalt = false;
					gimeBus->cpu.cpuHaltAsserted = false;
				}
				return FDC_OP_WRITE_TRACK;
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
	fdcStatusReg = FDC_STATUS_I_BUSY;		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
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
	fdcStatusReg = FDC_STATUS_I_BUSY;		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
	stepWaitCounter = curStepRate;
}

void FD502::fdcStep(uint8_t commandByte)
{
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	updateTrackRegFlag = commandByte & 0b00010000;
	fdcPendingCommand = FDC_OP_STEP;
	fdcStatusReg = FDC_STATUS_I_BUSY;		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
	stepWaitCounter = curStepRate;
}

void FD502::fdcStepIn(uint8_t commandByte)
{
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	updateTrackRegFlag = commandByte & 0b00010000;
	fdcPendingCommand = FDC_OP_STEP_IN;
	fdcStatusReg = FDC_STATUS_I_BUSY;		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
	stepWaitCounter = curStepRate;
}

void FD502::fdcStepOut(uint8_t commandByte)
{
	curStepRate = fdcStepRateTable[commandByte & 0x03];
	verifyAfterCommand = commandByte & 0b00000100;
	updateTrackRegFlag = commandByte & 0b00010000;
	fdcPendingCommand = FDC_OP_STEP_OUT;
	fdcStatusReg = FDC_STATUS_I_BUSY;		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
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
		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
		fdcStatusReg = FDC_STATUS_II_III_BUSY;		
		bufferPosition = 0;
		fdcByteDelayCounter = 0;
		fdcPendingCommand = FDC_OP_READ_SECTOR;
		//stepWaitCounter = curStepRate;

		// FOR DEBUG ONLY
		checksumByte = 0;
	}
	else
	{
		fdcStatusReg = (FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_RECORD_NOT_FOUND);
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
		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
		fdcStatusReg = FDC_STATUS_II_III_BUSY;
		bufferPosition = 0;
		fdcByteDelayCounter = 0;
		fdcPendingCommand = FDC_OP_WRITE_SECTOR;
	}
	else
	{
		fdcStatusReg = (FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_RECORD_NOT_FOUND);
		fdcPendingCommand = FDC_OP_WRITE_SECTOR;
		printf("FDC: Record Not Found\n");
	}
	fdcHeadLoaded = false;

	// FOR DEBUG ONLY
	//gimeBus->cpu.debuggerStepEnabled = true;

	return 0;
}

uint8_t FD502::fdcWriteTrack()
{
	if (fdcDrive[fdcDriveSelect].isDiskInserted)
	{
		// Note: When starting a command, we aren't merging bits into status register but starting fresh with default value
		fdcStatusReg = (FDC_STATUS_II_III_BUSY | FDC_STATUS_II_III_DATA_REQUEST);
		curFormatSegment = FDC_FORMAT_SEGMENT_GAP5;
		formatSectorInfoIndex = 0;
		bufferPosition = 0;
		formatSectorByteCount = 0;
		formatTotalBytesCounter = 0;
		fdcByteDelayCounter = 0;
		formatWritingStarted = false;
		fdcPendingCommand = FDC_OP_WRITE_TRACK;
		printf("FDC: Formatting Track %02u Side %01u\n", fdcTrackReg, fdcSideSelect);
		if (fdcDrive[fdcDriveSelect].diskImageGeometry->totalSides > 1)
			snprintf(textBuffer, sizeof(textBuffer), "FDC: Formatting Track %02u Side %01u", fdcTrackReg, fdcSideSelect);
		else
			snprintf(textBuffer, sizeof(textBuffer), "FDC: Formatting Track %02u", fdcTrackReg);
		gimeBus->statusBarText = textBuffer;
	}
	else
		return FD502_ERROR_NO_DISK_INSERTED;
}