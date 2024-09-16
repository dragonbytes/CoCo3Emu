#pragma once
#include <cstdint>
#include <string>

constexpr uint8_t FDC_CMD_PREFIX_RESTORE_SEEK				= 0b00000000;
constexpr uint8_t FDC_CMD_PREFIX_STEP						= 0b00100000;
constexpr uint8_t FDC_CMD_PREFIX_STEP_IN					= 0b01000000;
constexpr uint8_t FDC_CMD_PREFIX_STEP_OUT					= 0b01100000;
constexpr uint8_t FDC_CMD_PREFIX_READ_SECTOR				= 0b10000000;
constexpr uint8_t FDC_CMD_PREFIX_WRITE_SECTOR				= 0b10100000;
constexpr uint8_t FDC_CMD_PREFIX_READ_ADDR_FORCE_INTERRUPT	= 0b11000000;
constexpr uint8_t FDC_CMD_PREFIX_READ_WRITE_TRACK			= 0b11100000;

constexpr uint8_t FDC_OP_NONE								= 0;
constexpr uint8_t FDC_OP_RESTORE							= 1;
constexpr uint8_t FDC_OP_SEEK								= 2;
constexpr uint8_t FDC_OP_STEP								= 3;
constexpr uint8_t FDC_OP_STEP_IN							= 4;
constexpr uint8_t FDC_OP_STEP_OUT							= 5;
constexpr uint8_t FDC_OP_READ_SECTOR						= 6;
constexpr uint8_t FDC_OP_WRITE_SECTOR						= 7;
constexpr uint8_t FDC_OP_READ_ADDRESS						= 8;
constexpr uint8_t FDC_OP_WRITE_ADDRESS						= 9;
constexpr uint8_t FDC_OP_READ_TRACK							= 10;
constexpr uint8_t FDC_OP_WRITE_TRACK						= 11;
constexpr uint8_t FDC_OP_FORCE_INTERRUPT					= 12;

constexpr uint8_t FDC_LAST_STEPPED_IN						= 0;
constexpr uint8_t FDC_LAST_STEPPED_OUT						= 1;

constexpr uint8_t FDC_STATUS_I_NOT_READY					= 0x80;
constexpr uint8_t FDC_STATUS_I_PROTECTED					= 0x40;
constexpr uint8_t FDC_STATUS_I_HEAD_LOADED					= 0x20;
constexpr uint8_t FDC_STATUS_I_SEEK_ERROR					= 0x10;
constexpr uint8_t FDC_STATUS_I_CRC_ERROR					= 0x08;
constexpr uint8_t FDC_STATUS_I_TRACK_00						= 0x04;
constexpr uint8_t FDC_STATUS_I_INDEX						= 0x02;
constexpr uint8_t FDC_STATUS_I_BUSY							= 0x01;

constexpr uint8_t FDC_STATUS_II_III_NOT_READY				= 0x80;
constexpr uint8_t FDC_STATUS_II_III_WRITE_PROTECT			= 0x40;
constexpr uint8_t FDC_STATUS_II_III_REC_TYPE_OR_WRITE_FAULT	= 0x20;
constexpr uint8_t FDC_STATUS_II_III_RECORD_NOT_FOUND		= 0x10;
constexpr uint8_t FDC_STATUS_II_III_CRC_ERROR				= 0x08;
constexpr uint8_t FDC_STATUS_II_III_LOST_DATA				= 0x04;
constexpr uint8_t FDC_STATUS_II_III_DATA_REQUEST			= 0x02;
constexpr uint8_t FDC_STATUS_II_III_BUSY					= 0x01;

constexpr uint8_t FD502_OPERATION_COMPLETE					= 0x00;
constexpr uint8_t FD502_HALT_ENABLED						= 0x01;
constexpr uint8_t FD502_ERROR_DRIVE_NOT_ATTACHED			= 0x02;
constexpr uint8_t FD502_ERROR_NO_DISK_INSERTED				= 0x03;
constexpr uint8_t FD502_ERROR_OPENING_DISK_IMAGE			= 0x04;
constexpr uint8_t FD502_ERROR_UNSUPPORTED_DISK_IMAGE		= 0x05;

struct diskImageStruct
{
	long totalFilesize;
	uint16_t bytesPerTrack;
	uint16_t bytesPerSector;
	uint8_t sectorsPerTrack;
	uint8_t tracksPerSide;
	uint8_t totalSides;
};

struct driveStruct
{
	uint8_t totalCylinders;
	bool isDoubleSided;
	bool isDoubleDensity;
	bool isDriveAttached;
	bool isDiskInserted;
	FILE* diskImageFile;
	diskImageStruct* diskImageGeometry;
};

class FD502
{
public:
	FD502();

	void ConnectToBus(GimeBus* busPtr) { gimeBus = busPtr; }
	void fdcAttachNewDrive(uint8_t, uint8_t, bool, bool);	// Params = Drive Number, Total Physical Tracks, Total Sides, Single or Double-densityu
	void fdcDetachDrive(uint8_t);
	uint8_t fdcInsertDisk(uint8_t, std::string);
	void fdcRegisterWrite(uint16_t, uint8_t);
	uint8_t fdcRegisterRead(uint16_t);
	uint8_t fdcHandleNextEvent();

public:
	bool isConnected;
	uint8_t fdcDriveSelect;
	bool fdcMotorOn;
	bool fdcWritePrecompensation;
	bool fdcDoubleDensity;					// false = Single Density, true = Double Density
	bool fdcHaltFlag;
	bool fdcHeadLoaded;
	uint8_t fdcCommandReg, fdcTrackReg, fdcSectorReg, fdcSideSelect, fdcDataReg, fdcStatusReg;
	uint8_t stepWaitCounter, curStepRate, fdcPendingCommand;
	driveStruct fdcDrive[3];
	char textBuffer[128];

	// FOR DEBUG
	uint8_t checksumByte;
	bool testFlag;

private:
	void fdcSeekCommand(uint8_t);
	void fdcRestoreCommand(uint8_t);
	void fdcStep(uint8_t);
	void fdcStepIn(uint8_t);
	void fdcStepOut(uint8_t);
	void fdcForceInterrupt(uint8_t);
	uint8_t fdcReadSector();
	uint8_t fdcWriteSector();
	uint32_t getDiskImageOffset(uint8_t, uint8_t, uint8_t, uint8_t);

	GimeBus* gimeBus = nullptr;
	uint8_t physicalTrackPosition, destinationTrack, destinationSector, lastStepDirection;
	uint16_t bufferPosition;
	uint8_t fdcStepRateTable[4] = { 6, 12, 20, 30 };
	uint8_t sectorBuffer[512];
	bool updateTrackRegFlag, verifyAfterCommand;

	// Common CoCo disk image configs
	// Elements = Total Filesize in bytes, Bytes/Sector, Sectors/Track, Tracks/Side, Total Sides
	diskImageStruct diskImageGeometry[5] = 
	{
		{ 161280, 4608, 256, 18, 35, 1 },
		{ 322560, 4608, 256, 18, 35, 2 },
		{ 184320, 4608, 256, 18, 40, 1 },
		{ 368640, 4608, 256, 18, 40, 2 },
		{ 737280, 4608, 256, 18, 80, 2 }
	};
};

