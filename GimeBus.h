#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "CPU6809.h"
#include "DeviceROM.h"
#include "olcPixelGameEngine.h"

constexpr uint8_t IRQ_SRC_PIA_VSYNC = 1;
constexpr uint8_t IRQ_SRC_PIA_HSYNC = 2;
constexpr uint8_t IRQ_SRC_GIME_VSYNC = 3;
constexpr uint8_t IRQ_SRC_GIME_HSYNC = 4;

constexpr uint8_t RESET_INT_SRC = 1;

// GIME master clock frequency in Hz
constexpr float gimeMasterClock_NTSC = (28636360.0F / 4.0F);
constexpr double gimeFrameTime_NTSC = gimeMasterClock_NTSC / 60;
constexpr float gimeMasterClock_PAL = 2847500;
constexpr double gimeFrameTime_PAL = gimeMasterClock_PAL / 60;
constexpr double cpuClockFreq_NTSC = gimeMasterClock_NTSC / 4;
constexpr double cpuFrameTime_NTSC = cpuClockFreq_NTSC / 60;

// GIME 64-color palette definition
const olc::Pixel gimePaletteRGB[64] = {

	{0, 0, 0},		{0, 0, 85},		{0, 85, 0},		{0, 85, 85},	{85, 0, 0},		{85, 0, 85},	{85, 85, 0},	{85, 85, 85},
	{0, 0, 170},	{0, 0, 255},	{0, 85, 170},	{0, 85, 255},	{85, 0, 170},	{85, 0, 255},	{85, 85, 170},	{85, 85, 255},
	{0, 170, 0},	{0, 170, 85},	{0, 255, 0},	{0, 255, 85},	{85, 170, 0},	{85, 170, 85},	{85, 255, 0},	{85, 255, 85},
	{0, 170, 170},	{0, 170, 255},	{0, 255, 170},	{0, 255, 255},	{85, 170, 170},	{85, 170, 255},	{85, 255, 170},	{85, 255, 255},
	{170, 0, 0},	{170, 0, 85},	{170, 85, 0},	{170, 85, 85},	{255, 0, 0},	{255, 0, 85},	{255, 85, 0},	{255, 85, 85},
	{170, 0, 170},	{170, 0, 255},	{170, 85, 170},	{170, 85, 255},	{255, 0, 170},	{255, 0, 255},	{255, 85, 170},	{255, 85, 255},
	{170, 170, 0},	{170, 170, 85},	{170, 255, 0},	{170, 255, 85},	{255, 170, 0},	{255, 170, 85},	{255, 255, 0},	{255, 255, 85},
	{170, 170, 170},{170, 170, 255},{170, 255, 170},{170, 255, 255},{255, 170, 170},{255, 170, 255},{255, 255, 170},{255, 255, 255}
};

//-------------------------------------------------
//  semigraphics4_fontdata8x12
//-------------------------------------------------

const uint8_t semigraphics4_fontdata8x12[] =
{
	/* Block Graphics (Semigraphics 4 Graphics ) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// GIME Register Definitions
constexpr uint8_t GIME_ROM_16_SPLIT		= 0b00000000;	// (Could also technically be 0b00000001)
constexpr uint8_t GIME_ROM_32_INTERNAL	= 0b00000010;
constexpr uint8_t GIME_ROM_32_EXTERNAL	= 0b00000011;

constexpr uint8_t PIA_CTRL_MUX_MASK			= 0b00001000;
constexpr uint8_t PIA_CTRL_DIR_MASK			= 0b00000100;
constexpr uint8_t PIA_CTRL_IRQEDGE_MASK		= 0b00000010;
constexpr uint8_t PIA_CTRL_IRQENABLE_MASK	= 0b00000001;

constexpr uint8_t COL7 = 0x80;
constexpr uint8_t COL6 = 0x40;
constexpr uint8_t COL5 = 0x20;
constexpr uint8_t COL4 = 0x10;
constexpr uint8_t COL3 = 0x08;
constexpr uint8_t COL2 = 0x04;
constexpr uint8_t COL1 = 0x02;
constexpr uint8_t COL0 = 0x01;
constexpr uint8_t ROW7 = 0x80;
constexpr uint8_t ROW6 = 0x40;
constexpr uint8_t ROW5 = 0x20;
constexpr uint8_t ROW4 = 0x10;
constexpr uint8_t ROW3 = 0x08;
constexpr uint8_t ROW2 = 0x04;
constexpr uint8_t ROW1 = 0x02;
constexpr uint8_t ROW0 = 0x01;

struct piaRegisters
{
	uint8_t controlReg;
	uint8_t dataDirReg;
	uint8_t dataReg;
};

struct piaDevice
{
	piaRegisters SideA;
	piaRegisters SideB;
};

class CoCoEmuPGE;

struct keyMatrix
{
	olc::Key key;
	uint8_t cocoKeyMask;
};

struct olcKeyMatrix
{
	olc::Key keyID;
	bool keyDownState;
};

class GimeBus
{
	public:
		GimeBus();

	public:
		Cpu6809 cpu;
		DeviceROM* romCoCo3;
		DeviceROM* romExternal;

		std::vector<uint8_t> physicalRAM;
		std::vector<olc::Pixel> offscreenBuffer;
		uint16_t ramTotalSizeKB;
		unsigned int masterBusCycleCount;
		unsigned int curScanline, dotCounter;	// 1024 GIME cycles for Active Display Area, 1484 GIME cycles for start of left border to end of right border, 1820 GIME cycles for EVERYTHING together, 336 GIME cycles of horizontal blanking period
		int cpuClockDivisor;
		uint32_t videoStartAddr;
		uint8_t gimePaletteRegs[16];

		olcKeyMatrix hostKeyMatrix[56] = {
			{olc::G, false},		{olc::O, false}, {olc::W, false}, {olc::SPACE, false},	{olc::K7, false}, {olc::OEM_2, false},	{olc::SHIFT, false},
			{olc::F, false},		{olc::N, false}, {olc::V, false}, {olc::RIGHT, false},	{olc::K6, false}, {olc::PERIOD, false},	{olc::F2, false},
			{olc::E, false},		{olc::M, false}, {olc::U, false}, {olc::LEFT, false},	{olc::K5, false}, {olc::EQUALS, false},	{olc::F1, false},
			{olc::D, false},		{olc::L, false}, {olc::T, false}, {olc::DOWN, false},	{olc::K4, false}, {olc::COMMA, false},	{olc::CTRL, false},
			{olc::C, false},		{olc::K, false}, {olc::S, false}, {olc::UP, false},		{olc::K3, false}, {olc::OEM_1, false},	{olc::INS, false},
			{olc::B, false},		{olc::J, false}, {olc::R, false}, {olc::Z, false},		{olc::K2, false}, {olc::MINUS, false},	{olc::ESCAPE, false},
			{olc::A, false},		{olc::I, false}, {olc::Q, false}, {olc::Y, false},		{olc::K1, false}, {olc::K9, false},		{olc::HOME, false},
			{olc::OEM_4, false},	{olc::H, false}, {olc::P, false}, {olc::X, false},		{olc::K0, false}, {olc::K8, false},		{olc::ENTER, false}
		};

		void ConnectBusGime(CoCoEmuPGE* inputPtr) { mainPtr = inputPtr; }
		uint8_t readPhysicalByte(uint16_t);
		uint8_t writePhysicalByte(uint16_t, uint8_t);
		uint8_t readMemoryByte(uint16_t);
		uint16_t readMemoryWord(uint16_t);
		uint8_t writeMemoryByte(uint16_t,uint8_t);
		uint16_t writeMemoryWord(uint16_t,uint16_t);
		void SetRAMSize(int);
		void gimeBusClockTick();
		void updateVideoParams();

	private:
		piaDevice devPIA0, devPIA1;
		CoCoEmuPGE* mainPtr = nullptr;

		void buildNewScanline(uint16_t);
		
		// GIME internal variables
		struct GimeInit0
		{
			bool cocoCompatMode;
			bool mmuEnabled;
			bool chipIRQEnabled;
			bool chipFIRQEnabled;
			bool constSecondaryVectors;
			bool scsEnabled;
			uint8_t romMapControl;
		} gimeRegInit0;

		struct GimeInit1
		{
			bool memoryType;		// False = 64K chips, True = 256K chips
			bool timerSource;		// False = 63.695 usec, True = 279.365 nsec
			uint8_t mmuTaskSelect;	// 0 = 0xFFA0-0xFFA7, 1 = 0xFFA8-0xFFAF
		} gimeRegInit1;

		struct GimeMMUBankReg
		{
			uint8_t bankNum;
			uint32_t mmuBlockAddr;
		};

		GimeMMUBankReg gimeMMUBankRegs[16] = 
		{	{0x38, 0x70000}, {0x39, 0x72000}, {0x3A, 0x74000}, {0x3B, 0x76000}, {0x3C, 0x78000}, {0x3D, 0x7A000}, {0x3E, 0x7C000}, {0x3F, 0x7E000}, 
			{0x38, 0x70000}, {0x39, 0x72000}, {0x3A, 0x74000}, {0x3B, 0x76000}, {0x3C, 0x78000}, {0x3D, 0x7A000}, {0x3E, 0x7C000}, {0x3F, 0x7E000}
		};

		bool gimeAllRamModeEnabled;
		uint8_t gimeVertOffsetMSB;
		uint8_t gimeVertOffsetLSB;

		uint8_t samPageSelectMasks[14] = { 0xFE, 0x01, 0xFD, 0x02, 0xFB, 0x04, 0xF7, 0x08, 0xEF, 0x10, 0xDF, 0x20, 0xBF, 0x40 };
		uint8_t samPageSelectReg;

		uint8_t getCocoKey(uint8_t);
		uint8_t cocoKeyStrobeResult[7] = { 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF };

		/*
	public:
		union SAMPageSelectRegs
		{
		public:
			struct SAMPageSelectBits
			{
				bool bitF0 : 1;
				bool bitF1 : 1;
				bool bitF2 : 1;
				bool bitF3 : 1;
				bool bitF4 : 1;
				bool bitF5 : 1;
				bool bitF6 : 1;
				bool bitF7 : 1;
			} bitArray;
			uint8_t pageSelectRegByte;
		} samPageSelectRegs;
		*/
};
