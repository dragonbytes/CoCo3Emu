#pragma once
#include "olcPixelGameEngine.h"
#include <cstdint>
#include <queue>
#include "GimeBus.h"
#include "DeviceROM.h"

constexpr int STATUSBAR_HEIGHT = 12;
constexpr int STATUSBAR_START_Y = 242;
constexpr int STATUSBAR_TEXT_OFFSET_X = 8;
constexpr int STATUSBAR_TEXT_OFFSET_Y = STATUSBAR_START_Y + 2;

constexpr float audioSampleRateInHz				= 48000.0f;
constexpr float gimeAudioCountInterval			= (gimeMasterClock_NTSC / audioSampleRateInHz);
constexpr unsigned int numSamplesToBuffer		= 400;

constexpr uint8_t CONFIG_SUCCESSFULLY_LOADED	= 0x00;
constexpr uint8_t CONFIG_ERROR_OPENING_FILE		= 0x01;
constexpr uint8_t CONFIG_ERROR_INVALID_SYNTAX	= 0x02;
constexpr uint8_t CONFIG_ERROR_MOUNTING_DISK	= 0x03;

struct configResultStruct
{
	bool validResult;
	std::string paramKeyword;
	std::string paramValue;
};

struct stereoSamnpleStruct
{
	uint8_t rightChannel;
	uint8_t leftChannel;
};

// Override base class with your custom functionality
class CoCoEmuPGE : public olc::PixelGameEngine
{
public:
	CoCoEmuPGE();

public:
	GimeBus gimeBus;
	float statusElapsedTime = 0, frameTimePeriod = (1.0f / 60.0f);
	float gimeAudioCounter = gimeAudioCountInterval, audioNextCoCoSample = 0;
	long double averageElapsedGimeCycles = 0;
	unsigned int averageElapsedGimeCyclesCounter = 0;
	long double fpsFrameCounter = 0, elapsedGimeClockCycles, residualCycles = 0, residualGimeCycleCount = 0;
	char vdgTextLine[32];
	bool frameRenderedFlag = false, oneTimeFlag = false;
	int cocoKeyCycleCounter = 0;
	bool keyBackspacePressed;
	unsigned int cmpOrRgb;		// 0 = Composite definitions, 1 = RGB definitions
	uint8_t curLinesPerRow, curCharsPerRow, curBytesPerCharRow, curBytesPerChar, underlineRowOffset, bytesPerPixelRow, pixelsPerDraw, colorSetOffset;
	olc::Pixel borderPixel;
	uint16_t dataBlockSize, blockLoadAddress, execAddress;
	std::string strStatusBarIdle = "Idle";
	std::string prevStatusBarMsg;
	bool statusBarTimeout;

	const unsigned int vdgColumnsPerRow = 32;

	bool OnUserCreate() override;
	bool OnUserDestroy() override;
	bool OnUserUpdate(float fElapsedTime) override;
	bool OnConsoleCommand(const std::string&) override;
	float SoundHandler(int, float, float);

	DeviceROM* loadFileROM(const char*);
	bool updateStatusBar();

private:
	uint32_t vdgVideoStartAddr, screenRamPtr;
	uint16_t rowPixelCounter, vdgFontDataIndex, scanlineIndex;
	uint8_t fontLineOffset, bottomLineOffset, curScreenChar, curScreenAttr, fontDataByte, topBorderEnd, bottomBorderStart, curScreenByte;
	olc::Pixel foregroundPixel, backgroundPixel, fontPixel, pixelArray[8];
	bool uiLoadmHasExecAddr;
	unsigned int commandStringIndex = 0;
	uint8_t cocoAudioSample;
	stereoSamnpleStruct newStereoSample;
	//std::queue<stereoSamnpleStruct> fifoAudioBuffer;
	std::queue<uint8_t> leftAudioBuffer;
	std::queue<uint8_t> rightAudioBuffer;
	bool audioBufferReady = false;

	float averageTime = 0;
	int numAverageSamples = 0;
	long float previousTime = 0, currentTime = 0;

	void renderScanlineVDG(unsigned int);
	void renderScanlineGIME(unsigned int);
	std::string nextStringWord(std::string);
	std::string stringToUpper(std::string);
	void commandLOADM(std::string);
	void commandEXEC();
	uint8_t loadConfigFile();
	configResultStruct parseConfigLine(std::string);
};