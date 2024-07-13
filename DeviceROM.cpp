#include "DeviceROM.h"

DeviceROM::DeviceROM(uint16_t romSizeBytes)
{
	romData.resize(romSizeBytes);
	// Set size and initilize ROM data block
	for (int i = 0; i < romSizeBytes; i++)
		romData[i] = std::rand() % 256;
}

uint8_t DeviceROM::readByte(uint16_t address)
{
	// Note: readRomSize MUST be set to a non-zero number prior to calling this function or an exception will occur
	return romData[address % readRomSize];	// This MOD operation with the ROM's size should force the value to keep rolling over until its valid with the ROM's range/size
}
