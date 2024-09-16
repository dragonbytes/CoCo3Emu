#pragma once
#include <vector>
#include <string>

class DeviceROM
{
public: 
	DeviceROM(uint16_t);

public:
	uint8_t readByte(uint16_t);

	std::vector<uint8_t> romData;
	uint16_t readRomSize;
};

