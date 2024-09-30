#pragma once

constexpr uint8_t EMUDISK_OPERATION_COMPLETE			= 0x00;
constexpr uint8_t EMUDISK_ERROR_NOT_ENABLED				= 0x02;
constexpr uint8_t EMUDISK_ERROR_NO_DISK_MOUNTED			= 0x03;
constexpr uint8_t EMUDISK_ERROR_OPENING_DISK_IMAGE		= 0x04;
constexpr uint8_t EMUDISK_ERROR_UNSUPPORTED_DISK_IMAGE	= 0x05;

constexpr uint8_t EMUDISK_STATUS_SUCCESSFUL				= 0;
constexpr uint8_t EMUDISK_STATUS_ERROR_NOT_ENABLED		= 2;
constexpr uint8_t EMUDISK_STATUS_ERROR_INVALID_DRV_NUM	= 27;
constexpr uint8_t EMUDISK_STATUS_ERROR_INVALID_CMD		= 254;

struct emuDiskDriveStruct
{
	bool imageMounted;
	long diskImageFilesize;
	std::string imgFilePathname;
	FILE* vhdImageFile;
};

class EmuDisk
{
public:
	EmuDisk();

	void ConnectToBus(GimeBus* busPtr) { gimeBus = busPtr; }
	uint8_t vhdMountDisk(uint8_t, std::string);
	uint8_t vhdEjectDisk(uint8_t);
	void registerWrite(uint16_t regAddress, uint8_t paramByte);
	uint8_t readSector();
	uint8_t writeSector();

	union
	{
		struct
		{
			uint8_t zeroByte;
			uint8_t lowByte;
			uint8_t middleByte;
			uint8_t highByte;
		};
		uint32_t byteOffset;
	} nextLogicalSector;

	union
	{
		struct
		{
			uint8_t lowByte;
			uint8_t highByte;
		};
		uint16_t bufferPtr;
	} dataBufferPtr;

	bool isEnabled;
	uint8_t vhdDriveNum, statusCode;
	emuDiskDriveStruct emuDiskDrive[2];
	char textBuffer[128];

private:
	GimeBus* gimeBus = nullptr;
	uint8_t sectorBuffer[256];
};
