#include "GimeBus.h"
#include "EmuDisk.h"

EmuDisk::EmuDisk()
{
	nextLogicalSector.zeroByte = 0;		// By the least significant byte always being zero, this effectively multiplies our LSN sector number by 256 which is our sector size in bytes
	isEnabled = false;
}

uint8_t EmuDisk::vhdMountDisk(uint8_t driveNum, std::string diskImageFilePath)
{
	emuDiskDrive[driveNum].vhdImageFile = fopen(diskImageFilePath.c_str(), "rb+");
	if (emuDiskDrive[driveNum].vhdImageFile == NULL)
	//if (fopen_s(&emuDiskDrive[driveNum].vhdImageFile, diskImageFilePath.c_str(), "rb+") != 0)
		return EMUDISK_ERROR_OPENING_DISK_IMAGE;

	fseek(emuDiskDrive[driveNum].vhdImageFile, 0, SEEK_END);
	emuDiskDrive[driveNum].diskImageFilesize = ftell(emuDiskDrive[driveNum].vhdImageFile);
	emuDiskDrive[driveNum].imageMounted = true;
	return EMUDISK_OPERATION_COMPLETE;
}

void EmuDisk::registerWrite(uint16_t regAddress, uint8_t paramByte)
{
	switch (regAddress)
	{
	case 0xFF80:
		// Logical Sector Number (1 of 3 bytes)
		nextLogicalSector.highByte = paramByte;
		break;
	case 0xFF81:
		// Logical Sector Number (2 of 3 bytes)
		nextLogicalSector.middleByte = paramByte;
		break;
	case 0xFF82:
		// Logical Sector Number (3 of 3 bytes)
		nextLogicalSector.lowByte = paramByte;
		break;
	case 0xFF83:
		// Command/Status Register
		switch (paramByte)
		{
		case 0x00:
			// Read Sector Command
			statusCode = readSector();
			break;
		case 0x01:
			// Write Sector Command
			statusCode = writeSector();
			break;
		case 0x02:
			// Close 
			break;
		default:
			statusCode = EMUDISK_STATUS_ERROR_INVALID_CMD;
			break;
		}
		break;
	case 0xFF84:
		// Pointer to Read/Write Buffer 
		dataBufferPtr.highByte = paramByte;
		break;
	case 0xFF85:
		dataBufferPtr.lowByte = paramByte;
		break;
	case 0xFF86:
		vhdDriveNum = paramByte;
		break;
	}
}

uint8_t EmuDisk::readSector()
{
	if (vhdDriveNum > 0x01)
		return EMUDISK_STATUS_ERROR_INVALID_DRV_NUM;					// Invalid Virtual HDD drive number
	else if (!emuDiskDrive[vhdDriveNum].imageMounted)
		return EMUDISK_STATUS_ERROR_NOT_ENABLED;
	else
	{
		if (nextLogicalSector.byteOffset <= (emuDiskDrive[vhdDriveNum].diskImageFilesize - 256))
		{
			// Requested LSN is not greater than our filesize and therefore valid. Seek to it and grab the 256-byte sector from .VHD file
			fseek(emuDiskDrive[vhdDriveNum].vhdImageFile, nextLogicalSector.byteOffset, SEEK_SET);
			fread(&sectorBuffer, 1, 256, emuDiskDrive[vhdDriveNum].vhdImageFile);
			snprintf(textBuffer, sizeof(textBuffer), "EmuDisk: Read from Logical Sector Number %02X%02X%02X", nextLogicalSector.highByte, nextLogicalSector.middleByte, nextLogicalSector.lowByte);
			gimeBus->statusBarText = textBuffer;
			// Now copy our populated internal sectorBuffer data to it's destination within CoCo memory space
			for (uint16_t i = 0; i < 256; i++)
				gimeBus->writePhysicalByte(dataBufferPtr.bufferPtr + i, sectorBuffer[i]);
		}
		else
		{
			// If the requested LSN does not exist within our .VHD file, return a zero-ed out sector (This is the behavoir i've observed MAME doing in this situation)
			for (uint16_t i = 0; i < 256; i++)
				gimeBus->writePhysicalByte(dataBufferPtr.bufferPtr + i, 0x00);
		}
		return EMUDISK_STATUS_SUCCESSFUL;
	}
}

uint8_t EmuDisk::writeSector()
{
	if (vhdDriveNum > 0x01)
		return EMUDISK_STATUS_ERROR_INVALID_DRV_NUM;					// Invalid Virtual HDD drive number
	else if (!emuDiskDrive[vhdDriveNum].imageMounted)
		return EMUDISK_STATUS_ERROR_NOT_ENABLED;
	else
	{
		if (nextLogicalSector.byteOffset <= (emuDiskDrive[vhdDriveNum].diskImageFilesize - 256))
		{
			// First copy our source sector data from CoCo memory space to our temp sectorBuffer
			for (uint16_t i = 0; i < 256; i++)
				sectorBuffer[i] = gimeBus->readPhysicalByte(dataBufferPtr.bufferPtr + i);
			// Requested LSN is not greater than our filesize and therefore valid. Seek to it's position and write the 256-byte sector to the .VHD file
			fseek(emuDiskDrive[vhdDriveNum].vhdImageFile, nextLogicalSector.byteOffset, SEEK_SET);
			fwrite(&sectorBuffer, 1, 256, emuDiskDrive[vhdDriveNum].vhdImageFile);
			snprintf(textBuffer, sizeof(textBuffer), "EmuDisk: Wrote to Logical Sector Number %02X%02X%02X", nextLogicalSector.highByte, nextLogicalSector.middleByte, nextLogicalSector.lowByte);
			gimeBus->statusBarText = textBuffer;
		}
		return EMUDISK_STATUS_SUCCESSFUL;
	}
}

